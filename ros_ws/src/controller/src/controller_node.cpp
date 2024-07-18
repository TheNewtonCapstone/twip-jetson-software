#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <iostream>
#include "rclcpp/rclcpp.hpp"
#include <cstdio>

#include <sensor_msgs/msg/joint_state.hpp>

#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/string.hpp"
#include "controler/include/onnx_handler"

using std::placeholders::_1;
using namespace std::chrono_literals;
class Controller : public rclcpp::Node {
public:
  Controller(const std::string model_path, const int num_observations, const int num_actions) 
    :Node("controller"), 
    model_(model_path, num_observations, num_actions){
    // create ros2 messages 
    imu_state_ = std::make_shared<sensor_msgs::msg::Imu>();

    RCLCPP_INFO(get_logger(), "Imu initialized");

    sub_ = create_subscription<sensor_msgs::msg::Imu>("imu_data", 10, 
              std::bind(&Controller::imu_callback,this, std::placeholders::_1));


    RCLCPP_INFO(get_logger(), "1113 ROS imu subscriber succesffuly created ");

  
  // initliase control loop timer
 timer_ =  create_wall_timer(5ms, std::bind(&Controller::control_loop, this));

  }

  void imu_callback(const sensor_msgs::msg::Imu::SharedPtr msg){
     imu_state_ = std::move(msg);

    // Log the IMU state
    // RCLCPP_INFO(get_logger(), "IMU values:");
    // RCLCPP_INFO(get_logger(), "Orientation - x: %f, y: %f, z: %f, w: %f", 
    //             imu_state_->orientation.x, 
    //             imu_state_->orientation.y, 
    //             imu_state_->orientation.z, 
    //             imu_state_->orientation.w);
  }
  void control_loop(){
    std::copy(&imu_state_->orientation.x,&imu_state_->orientation.x+4,model_.input_buffer_.begin());
    model_.run();
    auto output = model_.output_buffer_;


    std::ostringstream out;
    out << "Control loop :\n";
    out << "Imu Quaternion Values\t" 
        << imu_state_->orientation.x << " "
        << imu_state_->orientation.y << " " 
        << imu_state_->orientation.z << " " 
        << imu_state_->orientation.w;
 



    out << "\nINPUTS:\t";
    for(auto element : model_.get_input_buffer()){ 
      out << element << " ";
    }
    out << "\nOUTPUTS\t";
    for(auto element : model_.get_output_buffer()){
      out << element << " ";
    }
    out << std::endl;


    // RCLCPP_INFO(get_logger(), "Control loop - x: %f, y: %f, z: %f, w: %f", 
    //             imu_state_->orientation.x, 
    //             imu_state_->orientation.y, 
    //             imu_state_->orientation.z, 
    //             imu_state_->orientation.w);

    RCLCPP_INFO(get_logger(), out.str().c_str());
    // model_.run();
    // auto output = "Control loop" + std::to_string(model_.output_buffer_[0]);
  }

  private: 

    //ros2 interfaces 
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr pub_;
    
    //ros2 messages
    sensor_msgs::msg::Imu::SharedPtr imu_state_;
    std::array<float, 4> imu_quaternion_;
    int num_observations_;
    int num_actions_;
    std::string model_path_;
    OnnxController model_;

};

/*
 * function configures the current process to use the SCHED_FIFO scheduling policy with a specified
 * real-time priority. It initializes the scheduling parameters, sets the priority, and applies the 
 * scheduling policy using the sched_setscheduler system call. If the priority setting fails, an error 
 * message is logged; otherwise, an informational message with the current priority is logged.
 * 
 * Parameters:
 * - controller: A shared pointer to the Controller class, which provides the logging mechanism.
 * 
 * Returns:
 * - void
*/

void set_real_time_priority(std::shared_ptr<Controller> controller){
  // initialize the scheduling parameters
  struct sched_param param;
  memset(&param, 0, sizeof(param));
  param.sched_priority = 98;


  //set the scheduling policy and priority for the current process 
  
  int ret =sched_setscheduler(getpid(), SCHED_FIFO, &param);
  if(ret != 0){
    RCLCPP_ERROR(controller ->get_logger(), "Failed to set the priority ");
  }else{
    std::string error = "Process priority is "  + std::to_string(param.sched_priority);
    RCLCPP_INFO(controller ->get_logger(), error.c_str());
    }
}

int main(int argc, char *argv[]) {
  std::cout << "Controller Node Started " << std::endl;

  rclcpp::init(argc, argv);

  char cwd[PATH_MAX];
  if(getcwd(cwd, sizeof(cwd)) != nullptr){
    std::cout << "Current Working directory is : " << cwd << std::endl;
  }else{
    perror("ERROR getting cwd");
  }

  std::string model_path = "src/controller/Twip.pth.onnx";
  int num_observations = 2;
  int num_actions =1;
  auto controller = std::make_shared<Controller>(model_path, num_observations,num_actions);

  set_real_time_priority(controller);

  rclcpp::executors::StaticSingleThreadedExecutor executor;
  executor.add_node(controller->get_node_base_interface());
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
