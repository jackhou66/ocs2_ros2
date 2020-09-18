/*
 * Author: Michael Spieler
 * Date:   2020-09-08
 */

#include <ocs2_mobile_manipulator_example/pinocchio_lib.h>

#include <pinocchio/parsers/urdf.hpp>

namespace pinocchio {

namespace urdf {

template Model& buildModel<double, 0, JointCollectionDefaultTpl>(const std::string& filename, const Model::JointModel& rootJoint,
                                                                 Model& model, const bool verbose);
template Model& buildModel<double, 0, JointCollectionDefaultTpl>(const std::string& filename, Model& model, const bool verbose);

template Model& buildModelFromXML<double, 0, JointCollectionDefaultTpl>(const std::string& filename, const Model::JointModel& rootJoint,
                                                                        Model& model, const bool verbose);
template Model& buildModelFromXML<double, 0, JointCollectionDefaultTpl>(const std::string& filename, Model& model, const bool verbose);

}  // namespace urdf

}  // namespace pinocchio
