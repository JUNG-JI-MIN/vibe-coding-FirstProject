#pragma once
#include "Blackboard.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace AI
{
    enum class Status
    {
        Success,
        Failure,
        Running
    };

    class Node
    {
    public:
        explicit Node(std::string name);

        virtual ~Node() = default;
        virtual Status Tick(Blackboard& context, float dt) = 0;

        virtual void Abort(Blackboard& context);

        const std::string name;
    };

    using NodePtr = std::unique_ptr<Node>;

    class Condition : public Node
    {
        std::function<bool(const Blackboard&)> predicate;

    public:
        Condition(std::string name, std::function<bool(const Blackboard&)> predicate);

        Status Tick(Blackboard& context, float) override;
    };

    class Action : public Node
    {
        std::function<Status(Blackboard&, float)> action;
        std::function<void(Blackboard&)> cancel;

    public:
        Action(std::string name, std::function<Status(Blackboard&, float)> action,
               std::function<void(Blackboard&)> cancel = {});

        Status Tick(Blackboard& context, float dt) override;

        void Abort(Blackboard& context) override;
    };

    // Reactive composites re-evaluate priority/conditions from the first child each tick.
    // When a higher priority branch wins, the previous Running action releases its reservation.
    class Composite : public Node
    {
    protected:
        std::vector<NodePtr> children;
        int running = -1;

        void Select(Blackboard& context, int selected);

    public:
        explicit Composite(std::string name);

        Composite& Add(NodePtr child);

        void Abort(Blackboard& context) override;
    };

    class Selector : public Composite
    {
    public:
        explicit Selector(std::string name);

        Status Tick(Blackboard& context, float dt) override;
    };

    class Sequence : public Composite
    {
    public:
        explicit Sequence(std::string name);

        Status Tick(Blackboard& context, float dt) override;
    };
} // namespace AI
