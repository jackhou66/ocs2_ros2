#include <ros/package.h>
#include <ros/ros.h>

#include <ocs2_anymal_croc/AnymalCrocInterface.h>
#include <ocs2_anymal_raisim/AnymalRaisimDummy.h>
#include <ocs2_mpc/MPC_Settings.h>

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    throw std::runtime_error("No task file specified. Aborting.");
  }
  const std::string taskName(argv[1]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic))

  // Initialize ros node
  ros::init(argc, argv, "anymal_croc_mrt");
  ros::NodeHandle nodeHandle;

  // Read URDF
  const std::string urdf_param = "/anymal_croc_raisim_urdf";
  std::string urdf;
  if (!ros::param::get(urdf_param, urdf)) {
    throw ros::Exception("Error reading ros parameter: " + urdf_param);
  }

  auto anymalInterface = anymal::getAnymalCrocInterface(taskName);

  // load settings
  ocs2::MPC_Settings mpcSettings;
  mpcSettings.loadSettings(anymal::getTaskFilePathCroc(taskName));
  ocs2::RaisimRolloutSettings raisimRolloutSettings;
  raisimRolloutSettings.loadSettings(ros::package::getPath("ocs2_anymal_croc_raisim") + "/config/raisim_rollout.info", "rollout");

  anymal::runAnymalRaisimDummy(nodeHandle, std::move(anymalInterface), urdf, mpcSettings.mrtDesiredFrequency_,
                               mpcSettings.mpcDesiredFrequency_, raisimRolloutSettings);

  return 0;
}
