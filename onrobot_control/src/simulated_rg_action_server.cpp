#include <algorithm>
#include <chrono>
#include <cmath>
#include <control_msgs/action/gripper_command.hpp>
#include <memory>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <string>
#include <vector>

#include "onrobot_control/command_codec.hpp"

class SimulatedRgActionServer final : public rclcpp::Node
{
public:
  using Action = control_msgs::action::GripperCommand;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Action>;
  using SteadyClock = std::chrono::steady_clock;

  SimulatedRgActionServer() : Node("onrobot_simulated_rg_action_server")
  {
    rcl_interfaces::msg::ParameterDescriptor read_only;
    read_only.read_only = true;
    read_only.description = "RG model used by the Gazebo aperture mapping";
    const auto model = declare_parameter<std::string>("model", "rg2", read_only);
    const auto limits = onrobot_control::rg_limits_for_model(model);
    if (!limits)
    {
      throw std::invalid_argument("model must be 'rg2' or 'rg6'");
    }
    max_position_m_ = static_cast<double>(limits->max_width) * 1.0e-4;

    const double default_open_angle = model == "rg2" ? -0.558505 : -0.628319;
    const double default_closed_angle = model == "rg2" ? 0.785398 : 0.628319;
    open_joint_position_ = declare_parameter<double>("open_joint_position", default_open_angle, read_only);
    closed_joint_position_ = declare_parameter<double>("closed_joint_position", default_closed_angle, read_only);
    joint_name_ = declare_parameter<std::string>("joint_name", "finger_joint", read_only);
    aperture_joint_name_ = declare_parameter<std::string>("aperture_joint_name", model + "_aperture", read_only);
    const auto goal_tolerance_m = declare_parameter<double>("goal_tolerance_m", 0.001);
    const auto goal_timeout_s = declare_parameter<double>("goal_timeout_s", 10.0);
    if (!std::isfinite(open_joint_position_) || !std::isfinite(closed_joint_position_) ||
        open_joint_position_ == closed_joint_position_ || !std::isfinite(goal_tolerance_m) || goal_tolerance_m <= 0.0 ||
        !std::isfinite(goal_timeout_s) || goal_timeout_s <= 0.0)
    {
      throw std::invalid_argument("joint endpoints, goal_tolerance_m, and goal_timeout_s must be finite and valid");
    }
    goal_tolerance_m_ = goal_tolerance_m;
    goal_timeout_ = std::chrono::duration<double>(goal_timeout_s);

    command_publisher_ = create_publisher<std_msgs::msg::Float64MultiArray>("joint_position_controller/commands", 10);
    aperture_publisher_ = create_publisher<sensor_msgs::msg::JointState>("rg_aperture/joint_states", 10);
    joint_state_subscription_ = create_subscription<sensor_msgs::msg::JointState>(
        "joint_states", 10, [this](sensor_msgs::msg::JointState::ConstSharedPtr state) { handle_joint_state(*state); });
    action_server_ = rclcpp_action::create_server<Action>(
        this, "gripper_command",
        [this](const rclcpp_action::GoalUUID&, std::shared_ptr<const Action::Goal> goal) { return handle_goal(goal); },
        [this](const std::shared_ptr<GoalHandle>&) { return handle_cancel(); },
        [this](const std::shared_ptr<GoalHandle>& goal_handle) { handle_accepted(goal_handle); });
    execution_timer_ = create_wall_timer(std::chrono::milliseconds(20), [this]() { update_action(); });
  }

private:
  rclcpp_action::GoalResponse handle_goal(const std::shared_ptr<const Action::Goal>& goal) const
  {
    if (!std::isfinite(goal->command.position) || goal->command.position < 0.0 ||
        goal->command.position > max_position_m_ || !std::isfinite(goal->command.max_effort) ||
        goal->command.max_effort < 0.0)
    {
      return rclcpp_action::GoalResponse::REJECT;
    }
    if (goal->command.max_effort > 0.0)
    {
      RCLCPP_WARN(get_logger(), "Simulation does not model force limits; send max_effort=0 to request position only");
      return rclcpp_action::GoalResponse::REJECT;
    }
    if (active_goal_ || !state_received_ || SteadyClock::now() - last_state_receipt_ > std::chrono::seconds(1) ||
        command_publisher_->get_subscription_count() == 0)
    {
      RCLCPP_WARN(get_logger(), "Rejecting simulated RG goal until joint-state feedback and Gazebo controller are "
                                "ready");
      return rclcpp_action::GoalResponse::REJECT;
    }
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel() const
  {
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandle>& goal_handle)
  {
    active_goal_ = goal_handle;
    target_aperture_m_ = goal_handle->get_goal()->command.position;
    target_joint_position_ = joint_position_for_aperture(target_aperture_m_);
    goal_start_ = SteadyClock::now();
    publish_joint_command(target_joint_position_);
  }

  void handle_joint_state(const sensor_msgs::msg::JointState& state)
  {
    const auto joint = std::find(state.name.begin(), state.name.end(), joint_name_);
    if (joint == state.name.end())
    {
      return;
    }
    const auto index = static_cast<std::size_t>(std::distance(state.name.begin(), joint));
    if (index >= state.position.size() || !std::isfinite(state.position[index]))
    {
      return;
    }

    current_joint_position_ = state.position[index];
    state_received_ = true;
    last_state_receipt_ = SteadyClock::now();

    sensor_msgs::msg::JointState aperture_state;
    aperture_state.header.stamp = now();
    aperture_state.name = { aperture_joint_name_ };
    aperture_state.position = { aperture_for_joint_position(current_joint_position_) };
    aperture_publisher_->publish(aperture_state);
  }

  double joint_position_for_aperture(double aperture_m) const
  {
    const double fraction = aperture_m / max_position_m_;
    return closed_joint_position_ + fraction * (open_joint_position_ - closed_joint_position_);
  }

  double aperture_for_joint_position(double joint_position) const
  {
    const double fraction = (joint_position - closed_joint_position_) / (open_joint_position_ - closed_joint_position_);
    return std::clamp(fraction, 0.0, 1.0) * max_position_m_;
  }

  void publish_joint_command(double joint_position)
  {
    std_msgs::msg::Float64MultiArray command;
    command.data = { joint_position };
    command_publisher_->publish(command);
  }

  void update_action()
  {
    if (!active_goal_)
    {
      return;
    }

    auto result = std::make_shared<Action::Result>();
    result->position = aperture_for_joint_position(current_joint_position_);
    result->effort = 0.0;
    result->stalled = false;
    result->reached_goal = std::abs(result->position - target_aperture_m_) <= goal_tolerance_m_;

    if (active_goal_->is_canceling())
    {
      publish_joint_command(current_joint_position_);
      active_goal_->canceled(result);
      active_goal_.reset();
      return;
    }
    if (!state_received_ || SteadyClock::now() - last_state_receipt_ > std::chrono::seconds(1))
    {
      publish_joint_command(current_joint_position_);
      active_goal_->abort(result);
      active_goal_.reset();
      return;
    }

    auto feedback = std::make_shared<Action::Feedback>();
    feedback->position = result->position;
    feedback->effort = 0.0;
    feedback->stalled = false;
    feedback->reached_goal = result->reached_goal;
    active_goal_->publish_feedback(feedback);

    if (result->reached_goal)
    {
      active_goal_->succeed(result);
      active_goal_.reset();
      return;
    }
    if (SteadyClock::now() - goal_start_ > goal_timeout_)
    {
      publish_joint_command(current_joint_position_);
      active_goal_->abort(result);
      active_goal_.reset();
    }
  }

  double max_position_m_{ 0.0 };
  double open_joint_position_{ 0.0 };
  double closed_joint_position_{ 0.0 };
  double goal_tolerance_m_{ 0.001 };
  double target_aperture_m_{ 0.0 };
  double target_joint_position_{ 0.0 };
  double current_joint_position_{ 0.0 };
  std::chrono::duration<double> goal_timeout_{ 10.0 };
  SteadyClock::time_point last_state_receipt_{};
  SteadyClock::time_point goal_start_{};
  bool state_received_{ false };
  std::string joint_name_;
  std::string aperture_joint_name_;
  std::shared_ptr<GoalHandle> active_goal_;
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr command_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr aperture_publisher_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_subscription_;
  rclcpp_action::Server<Action>::SharedPtr action_server_;
  rclcpp::TimerBase::SharedPtr execution_timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SimulatedRgActionServer>());
  rclcpp::shutdown();
  return 0;
}