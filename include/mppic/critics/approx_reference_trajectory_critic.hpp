// Copyright 2022 FastSense, Samsung Research
#pragma once

#include "mppic/critic_function.hpp"
#include "mppic/models/state.hpp"
#include "mppic/utils.hpp"

namespace mppi::critics
{

class ApproxReferenceTrajectoryCritic : public CriticFunction
{
public:
  void initialize() override;

  /**
   * @brief Evaluate cost related to trajectories path alignment using
   * approximate path to segment function
   *
   * @param costs [out] add reference cost values to this tensor
   */
  void score(
    const geometry_msgs::msg::PoseStamped & /*robot_pose*/,
    const models::State & state, const xt::xtensor<double, 3> & trajectories,
    const xt::xtensor<double, 2> & path, xt::xtensor<double, 1> & costs,
    nav2_core::GoalChecker * goal_checker) override;

protected:
  unsigned int power_{0};
  double weight_{0};
};

}  // namespace mppi::critics
