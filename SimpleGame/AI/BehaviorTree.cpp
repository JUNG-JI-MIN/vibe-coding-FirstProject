#include "stdafx.h"
#include "BehaviorTree.h"

namespace AI
{
    Node::Node(std::string name) : name(std::move(name))
    {
    }

    void Node::Abort(Blackboard& context)
    {
    }

    Condition::Condition(std::string name, std::function<bool(const Blackboard&)> predicate)
        : Node(std::move(name)), predicate(std::move(predicate))
    {
    }

    Status Condition::Tick(Blackboard& context, float)
    {
        return predicate(context) ? Status::Success : Status::Failure;
    }

    Action::Action(std::string name, std::function<Status(Blackboard&, float)> action,
                   std::function<void(Blackboard&)> cancel)
        : Node(std::move(name)), action(std::move(action)), cancel(std::move(cancel))
    {
    }

    Status Action::Tick(Blackboard& context, float dt)
    {
        context.activeAction = this->name;
        return action(context, dt);
    }

    void Action::Abort(Blackboard& context)
    {
        if (cancel)
        {
            cancel(context);
        }
    }

    void Composite::Select(Blackboard& context, int selected)
    {
        if (running >= 0 && running != selected)
        {
            children[running]->Abort(context);
        }
        running = selected;
    }

    Composite::Composite(std::string name) : Node(std::move(name))
    {
    }

    Composite& Composite::Add(NodePtr child)
    {
        children.push_back(std::move(child));
        return *this;
    }

    void Composite::Abort(Blackboard& context)
    {
        Select(context, -1);
    }

    Selector::Selector(std::string name) : Composite(std::move(name))
    {
    }

    Status Selector::Tick(Blackboard& context, float dt)
    {
        for (int i = 0; i < int(this->children.size()); ++i)
        {
            Status result = this->children[i]->Tick(context, dt);
            if (result != Status::Failure)
            {
                this->Select(context, result == Status::Running ? i : -1);
                return result;
            }
        }
        this->Select(context, -1);
        return Status::Failure;
    }

    Sequence::Sequence(std::string name) : Composite(std::move(name))
    {
    }

    Status Sequence::Tick(Blackboard& context, float dt)
    {
        for (int i = 0; i < int(this->children.size()); ++i)
        {
            Status result = this->children[i]->Tick(context, dt);
            if (result != Status::Success)
            {
                this->Select(context, result == Status::Running ? i : -1);
                return result;
            }
        }
        this->Select(context, -1);
        return Status::Success;
    }

} // namespace AI
