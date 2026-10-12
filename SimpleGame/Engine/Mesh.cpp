#include "stdafx.h"
#include "Mesh.h"

namespace Engine
{
    Material Mesh::Fallback(const std::string& name)
    {
        Material m;
        if (name.find("Floor") != std::string::npos)
        {
            m = {.17f, .23f, .26f, .65f, .75f, 0};
        }
        if (name.find("Wall") != std::string::npos)
        {
            m = {.42f, .47f, .49f, .8f, .5f, 0};
        }
        if (name.find("Power") != std::string::npos)
        {
            m = {.38f, .28f, .13f, .7f, .55f, 0};
        }
        if (name.find("LifeSupport") != std::string::npos)
        {
            m = {.22f, .4f, .33f, .6f, .55f, 0};
        }
        if (name.find("Medical") != std::string::npos)
        {
            m = {.51f, .63f, .63f, .4f, .6f, 0};
        }
        if (name.find("Research") != std::string::npos)
        {
            m = {.26f, .46f, .57f, .6f, .48f, 0};
        }
        if (name.find("Label") != std::string::npos)
        {
            m = {.23f, .76f, .81f, 0, .5f, 2};
        }
        return m;
    }

    void Mesh::Face(const std::vector<Vec3>& points, const std::vector<Vec3>& normals,
                    const std::string& group, const std::string& material)
    {
        if (points.size() < 3)
        {
            return;
        }
        Vec3 normal;
        for (size_t i = 0; i < points.size(); ++i)
        {
            Vec3 a = points[i], b = points[(i + 1) % points.size()];
            normal = normal + Cross(a - points[0], b - points[0]);
        }
        normal = Normal(normal);
        int axis = std::fabs(normal.x) > std::fabs(normal.y) ? 0 : 1;
        if (std::fabs(normal.z) > (axis == 0 ? std::fabs(normal.x) : std::fabs(normal.y)))
        {
            axis = 2;
        }
        auto project = [&](Vec3 p)
        {
            return axis == 0 ? Vec3{p.y, 0, p.z} : (axis == 1 ? Vec3{p.x, 0, p.z} : Vec3{p.x, 0, p.y});
        };
        std::vector<int> indices;
        float area = 0;
        for (int i = 0; i < int(points.size()); ++i)
        {
            indices.push_back(i);
            Vec3 a = project(points[i]), b = project(points[(i + 1) % points.size()]);
            Vec3 origin = project(points[0]);
            area += (a.x - origin.x) * (b.z - origin.z) - (b.x - origin.x) * (a.z - origin.z);
        }
        float sign = area >= 0 ? 1.f : -1.f;
        auto& part = parts[material];
        part.material = Fallback(group + material);
        int guard = int(points.size() * points.size());
        auto emit = [&](int a, int b, int c)
        {
            if (Length(Cross(points[b] - points[a], points[c] - points[a])) < 1e-7f)
            {
                return;
            }
            for (int j : {a, b, c})
            {
                part.vertices.push_back({points[j], Length(normals[j]) > .001f ? normals[j] : normal});
            }
            triangles.push_back({points[a], points[b], points[c], group});
        };
        while (indices.size() >= 3 && guard-- > 0)
        {
            if (indices.size() == 3)
            {
                emit(indices[0], indices[1], indices[2]);
                break;
            }
            bool cut = false;
            for (int i = 0; i < int(indices.size()); ++i)
            {
                int a = indices[(i + indices.size() - 1) % indices.size()], b = indices[i],
                    c = indices[(i + 1) % indices.size()];
                Vec3 pa = project(points[a]), pb = project(points[b]), pc = project(points[c]);
                if (Side(pa, pb, pc) * sign <= .000001f)
                {
                    continue;
                }
                bool occupied = false;
                for (int j : indices)
                {
                    Vec3 point = project(points[j]);
                    float x = Side(pa, pb, point), y = Side(pb, pc, point), z = Side(pc, pa, point);
                    bool inside =
                        !((x < -1e-9f || y < -1e-9f || z < -1e-9f) && (x > 1e-9f || y > 1e-9f || z > 1e-9f));
                    if (j != a && j != b && j != c && inside)
                    {
                        occupied = true;
                        break;
                    }
                }
                if (occupied)
                {
                    continue;
                }
                emit(a, b, c);
                indices.erase(indices.begin() + i);
                cut = true;
                break;
            }
            if (!cut)
            {
                // Collinear polygon points are common in exported corridor floors.
                bool removed = false;
                for (int i = 0; i < int(indices.size()); ++i)
                {
                    int a = indices[(i + indices.size() - 1) % indices.size()], b = indices[i],
                        c = indices[(i + 1) % indices.size()];
                    if (std::fabs(Side(project(points[a]), project(points[b]), project(points[c]))) <
                        .000001f)
                    {
                        indices.erase(indices.begin() + i);
                        removed = true;
                        break;
                    }
                }
                if (!removed)
                {
                    if (indices.size() == 4)
                    {
                        // A few exported perimeter caps are self-crossing quads. Preserve their
                        // diagonal split; floor concave polygons still use ear clipping above.
                        emit(indices[0], indices[1], indices[2]);
                        emit(indices[0], indices[2], indices[3]);
                        ++repairedFaces;
                        break;
                    }
                    throw std::runtime_error("Cannot triangulate OBJ polygon in " + group);
                }
            }
        }
    }

    Mesh::~Mesh()
    {
        Release();
    }

    void Mesh::LoadObj(const std::wstring& path)
    {
        std::ifstream file(path.c_str());
        if (!file)
        {
            throw std::runtime_error("Station OBJ file could not be opened");
        }
        std::vector<Vec3> positions, normals;
        std::string line, group = "Station", material = "Default";
        while (std::getline(file, line))
        {
            std::istringstream row(line);
            std::string kind;
            row >> kind;
            if (kind == "v" || kind == "vn")
            {
                Vec3 p;
                row >> p.x >> p.y >> p.z;
                (kind == "v" ? positions : normals).push_back(p);
            }
            else if (kind == "o" || kind == "g")
            {
                row >> group;
            }
            else if (kind == "usemtl")
            {
                row >> material;
            }
            else if (kind == "f" && group.find("Player175cm") != 0)
            {
                std::vector<Vec3> p, n;
                std::string token;
                while (row >> token)
                {
                    size_t slash = token.find('/'), last = token.find_last_of('/');
                    int index = std::stoi(token.substr(0, slash));
                    index = index > 0 ? index - 1 : int(positions.size()) + index;
                    if (index < 0 || index >= int(positions.size()))
                    {
                        throw std::runtime_error("Invalid OBJ vertex index");
                    }
                    Vec3 normal;
                    if (last != std::string::npos && last != slash && last + 1 < token.size())
                    {
                        int ni = std::stoi(token.substr(last + 1));
                        ni = ni > 0 ? ni - 1 : int(normals.size()) + ni;
                        if (ni < 0 || ni >= int(normals.size()))
                        {
                            throw std::runtime_error("Invalid OBJ normal index");
                        }
                        normal = normals[ni];
                    }
                    p.push_back(positions[index]);
                    n.push_back(normal);
                    groups[group].Include(positions[index]);
                }
                Face(p, n, group, material);
            }
        }
        if (triangles.empty())
        {
            throw std::runtime_error("OBJ contains no usable triangles");
        }
        if (repairedFaces)
        {
            std::fprintf(stderr, "OBJ: repaired %d non-simple quad caps.\n", repairedFaces);
        }
    }

    void Mesh::Cube()
    {
        const Vec3 p[8] = {{-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {-.5f, .5f, -.5f},
                           {-.5f, -.5f, .5f},  {.5f, -.5f, .5f},  {.5f, .5f, .5f},  {-.5f, .5f, .5f}};
        const int faces[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
                                 {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2}};
        for (const auto& face : faces)
        {
            std::vector<Vec3> points;
            for (int i : face)
            {
                points.push_back(p[i]);
            }
            Face(points, std::vector<Vec3>(4), "Cube", "Cube");
        }
    }

    void Mesh::Draw(MetalMaterial& shader, bool hdr, const Material* overrideMaterial)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        for (auto& entry : parts)
        {
            auto& part = entry.second;
            if (part.vertices.empty())
            {
                continue;
            }
            if (!part.buffer)
            {
                glGenBuffers(1, &part.buffer);
                glBindBuffer(GL_ARRAY_BUFFER, part.buffer);
                glBufferData(GL_ARRAY_BUFFER, part.vertices.size() * sizeof(Vertex), part.vertices.data(),
                             GL_STATIC_DRAW);
            }
            else
            {
                glBindBuffer(GL_ARRAY_BUFFER, part.buffer);
            }
            const auto& m = overrideMaterial ? *overrideMaterial : part.material;
            if (hdr)
            {
                shader.Use(m.metal, m.rough, m.glow, true);
            }
            else
            {
                glUseProgram(0);
                if (m.glow > 0)
                {
                    glDisable(GL_LIGHTING);
                }
                else
                {
                    glEnable(GL_LIGHTING);
                }
            }
            glColor3f(m.r, m.g, m.b);
            glVertexPointer(3, GL_FLOAT, sizeof(Vertex), nullptr);
            glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(sizeof(Vec3)));
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(part.vertices.size()));
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    void Mesh::Release()
    {
        for (auto& entry : parts)
        {
            if (entry.second.buffer)
            {
                glDeleteBuffers(1, &entry.second.buffer);
                entry.second.buffer = 0;
            }
        }
    }

} // namespace Engine
