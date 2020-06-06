#include "integration.h"
#include <utility>

Integration::Integration(ros::NodeHandle nh) : nh_(nh)
{
    sub_ = nh_.subscribe("vision_poses", 1000, &Integration::visionCallback, this);
    client_ = nh_.serviceClient<grasping::move>("grasping_service");
}
Integration::~Integration() {}

void Integration::setSurface(const geometry_msgs::Pose &surface)
{
    surface_ = surface;
}
void Integration::setBottleOffset(const geometry_msgs::Point &offset)
{
    bottle_offset_ = offset;
}

void Integration::visionCallback(const geometry_msgs::PoseArrayConstPtr &msg)
{
    ROS_INFO_STREAM("Recieved new bottle coordinates");

    std::vector<geometry_msgs::Pose> bottles = msg->poses;
    std::deque<std::pair<geometry_msgs::Pose, geometry_msgs::Pose>> moved_bottles;
    geometry_msgs::Point offset;
    offset.x = 0;
    offset.y = 0;
    offset.z = 0;
    for (auto bottle : bottles)
    {
        grasping::move move;
        move.request.current = bottle;
        move.request.target = surface_;
        addPoints(move.request.target.position, offset);

        ROS_INFO_STREAM("Bottle: (" << bottle.position.x << ", " << bottle.position.y << ", " << bottle.position.z << ")");

        if (client_.call(move))
        {
            if (move.response.success)
            {
                moved_bottles.push_back({move.request.current, move.request.target});
            }
            else
            {
                ROS_WARN_STREAM("Gripper could not move to bottle!");
            }
        }
        else
        {
            ROS_WARN_STREAM("Could not send move command to gripper!");
        }

        //shift offset
        addPoints(offset, bottle_offset_);
    }

    for (auto bottle : moved_bottles)
    {
        grasping::move move;
        move.request.current = bottle.second;
        move.request.target = bottle.first;

        if (client_.call(move))
        {
            if (!move.response.success)
            {
                ROS_WARN_STREAM("Gripper could not move to bottle!");
            }
        }
        else
        {
            ROS_WARN_STREAM("Could not send move command to gripper!");
        }
    }
}
void Integration::addPoints(geometry_msgs::Point &pt1, const geometry_msgs::Point &pt2)
{
    pt1.x += pt2.x;
    pt1.y += pt2.y;
    pt1.z += pt2.z;
}