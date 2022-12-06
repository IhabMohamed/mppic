// Copyright (c) 2022 Samsung Research America, @artofnothingness Alexey Budyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <thread>

#include "gtest/gtest.h"
#include "rclcpp/rclcpp.hpp"
#include "mppic/tools/utils.hpp"
#include "mppic/models/path.hpp"

// Tests noise generator object

class RosLockGuard
{
public:
  RosLockGuard() {rclcpp::init(0, nullptr);}
  ~RosLockGuard() {rclcpp::shutdown();}
};
RosLockGuard g_rclcpp;

using namespace mppi::utils;  // NOLINT
using namespace mppi;  // NOLINT

class TestGoalChecker : public nav2_core::GoalChecker
{
public:
  TestGoalChecker() {}

  virtual void initialize(
    const rclcpp_lifecycle::LifecycleNode::WeakPtr & /*parent*/,
    const std::string & /*plugin_name*/,
    const std::shared_ptr<nav2_costmap_2d::Costmap2DROS>/*costmap_ros*/) {}

  virtual void reset() {}

  virtual bool isGoalReached(
    const geometry_msgs::msg::Pose & /*query_pose*/,
    const geometry_msgs::msg::Pose & /*goal_pose*/,
    const geometry_msgs::msg::Twist & /*velocity*/) {return false;}

  virtual bool getTolerances(
    geometry_msgs::msg::Pose & pose_tolerance,
    geometry_msgs::msg::Twist & /*vel_tolerance*/)
  {
    pose_tolerance.position.x = 0.25;
    pose_tolerance.position.y = 0.25;
    return true;
  }
};

TEST(UtilsTests, MarkerPopulationUtils)
{
  auto pose = createPose(1.0, 2.0, 3.0);
  EXPECT_EQ(pose.position.x, 1.0);
  EXPECT_EQ(pose.position.y, 2.0);
  EXPECT_EQ(pose.position.z, 3.0);
  EXPECT_EQ(pose.orientation.w, 1.0);

  auto scale = createScale(1.0, 2.0, 3.0);
  EXPECT_EQ(scale.x, 1.0);
  EXPECT_EQ(scale.y, 2.0);
  EXPECT_EQ(scale.z, 3.0);

  auto color = createColor(1.0, 2.0, 3.0, 0.0);
  EXPECT_EQ(color.r, 1.0);
  EXPECT_EQ(color.g, 2.0);
  EXPECT_EQ(color.b, 3.0);
  EXPECT_EQ(color.a, 0.0);

  auto marker = createMarker(999, pose, scale, color, "map");
  EXPECT_EQ(marker.header.frame_id, "map");
  EXPECT_EQ(marker.id, 999);
  EXPECT_EQ(marker.pose, pose);
  EXPECT_EQ(marker.scale, scale);
  EXPECT_EQ(marker.color, color);
}

TEST(UtilsTests, ConversionTests)
{
  geometry_msgs::msg::TwistStamped output;
  builtin_interfaces::msg::Time time;

  // Check population is correct
  output = toTwistStamped(0.5, 0.3, time, "map");
  EXPECT_NEAR(output.twist.linear.x, 0.5, 1e-6);
  EXPECT_NEAR(output.twist.linear.y, 0.0, 1e-6);
  EXPECT_NEAR(output.twist.angular.z, 0.3, 1e-6);
  EXPECT_EQ(output.header.frame_id, "map");
  EXPECT_EQ(output.header.stamp, time);

  output = toTwistStamped(0.5, 0.4, 0.3, time, "map");
  EXPECT_NEAR(output.twist.linear.x, 0.5, 1e-6);
  EXPECT_NEAR(output.twist.linear.y, 0.4, 1e-6);
  EXPECT_NEAR(output.twist.angular.z, 0.3, 1e-6);
  EXPECT_EQ(output.header.frame_id, "map");
  EXPECT_EQ(output.header.stamp, time);

  nav_msgs::msg::Path path;
  path.poses.resize(5);
  path.poses[2].pose.position.x = 5;
  path.poses[2].pose.position.y = 50;
  models::Path path_t = toTensor(path);

  // Check population is correct
  EXPECT_EQ(path_t.x.shape(0), 5u);
  EXPECT_EQ(path_t.y.shape(0), 5u);
  EXPECT_EQ(path_t.yaws.shape(0), 5u);
  EXPECT_EQ(path_t.x(2), 5);
  EXPECT_EQ(path_t.y(2), 50);
  EXPECT_NEAR(path_t.yaws(2), 0.0, 1e-6);
}

TEST(UtilsTests, WithTolTests)
{
  geometry_msgs::msg::Pose pose;
  pose.position.x = 10.0;
  pose.position.y = 1.0;

  nav2_core::GoalChecker * goal_checker = new TestGoalChecker;

  // Test not in tolerance
  nav_msgs::msg::Path path;
  path.poses.resize(2);
  path.poses[1].pose.position.x = 0.0;
  path.poses[1].pose.position.y = 0.0;
  models::Path path_t = toTensor(path);
  EXPECT_FALSE(withinPositionGoalTolerance(goal_checker, pose, path_t));
  EXPECT_FALSE(withinPositionGoalTolerance(0.25, pose, path_t));

  // Test in tolerance
  path.poses[1].pose.position.x = 9.8;
  path.poses[1].pose.position.y = 0.95;
  path_t = toTensor(path);
  EXPECT_TRUE(withinPositionGoalTolerance(goal_checker, pose, path_t));
  EXPECT_TRUE(withinPositionGoalTolerance(0.25, pose, path_t));

  path.poses[1].pose.position.x = 10.0;
  path.poses[1].pose.position.y = 0.76;
  path_t = toTensor(path);
  EXPECT_TRUE(withinPositionGoalTolerance(goal_checker, pose, path_t));
  EXPECT_TRUE(withinPositionGoalTolerance(0.25, pose, path_t));

  path.poses[1].pose.position.x = 9.76;
  path.poses[1].pose.position.y = 1.0;
  path_t = toTensor(path);
  EXPECT_TRUE(withinPositionGoalTolerance(goal_checker, pose, path_t));
  EXPECT_TRUE(withinPositionGoalTolerance(0.25, pose, path_t));

  delete goal_checker;
  goal_checker = nullptr;
  EXPECT_FALSE(withinPositionGoalTolerance(goal_checker, pose, path_t));
}

TEST(UtilsTests, AnglesTests)
{
  // Test angle normalization by creating insane angles
  xt::xtensor<float, 1> angles, zero_angles;
  angles = xt::ones<float>({100});
  for (unsigned int i = 0; i != angles.shape(0); i++) {
    angles(i) = i * i;
    if (i % 2 == 0) {
      angles(i) *= -1;
    }
  }

  auto norm_ang = normalize_angles(angles);
  for (unsigned int i = 0; i != norm_ang.shape(0); i++) {
    EXPECT_TRUE((norm_ang(i) >= -M_PI) && (norm_ang(i) <= M_PI));
  }

  // Test shortest angular distance
  zero_angles = xt::zeros<float>({100});
  auto ang_dist = shortest_angular_distance(angles, zero_angles);
  for (unsigned int i = 0; i != ang_dist.shape(0); i++) {
    EXPECT_TRUE((ang_dist(i) >= -M_PI) && (ang_dist(i) <= M_PI));
  }

  // Test point-pose angle
  geometry_msgs::msg::Pose pose;
  pose.position.x = 0.0;
  pose.position.y = 0.0;
  pose.orientation.w = 1.0;
  double point_x = 1.0, point_y = 0.0;
  EXPECT_NEAR(posePointAngle(pose, point_x, point_y), 0.0, 1e-6);
}

