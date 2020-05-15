/******************************************************************************
Copyright (c) 2020, Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <iostream>

#include <ocs2_core/control/LinearController.h>

namespace ocs2 {

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
LinearController::LinearController(size_t stateDim, size_t inputDim) : ControllerBase(stateDim, inputDim) {}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
LinearController::LinearController(size_t stateDim, size_t inputDim, const scalar_array_t& controllerTime,
                                   const vector_array_t& controllerBias, const matrix_array_t& controllerGain)
    : LinearController(stateDim, inputDim) {
  setController(controllerTime, controllerBias, controllerGain);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
LinearController::LinearController(const LinearController& other)
    : LinearController(other.stateDim_, other.stateDim_, other.timeStamp_, other.biasArray_, other.gainArray_) {
  deltaBiasArray_ = other.deltaBiasArray_;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
LinearController& LinearController::operator=(LinearController other) {
  other.swap(*this);
  return *this;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
LinearController* LinearController::clone() const {
  return new LinearController(*this);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::setController(const scalar_array_t& controllerTime, const vector_array_t& controllerBias,
                                     const matrix_array_t& controllerGain) {
  timeStamp_ = controllerTime;
  biasArray_ = controllerBias;
  gainArray_ = controllerGain;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
vector_t LinearController::computeInput(const scalar_t& t, const vector_t& x) {
  vector_t uff;
  const auto indexAlpha = LinearInterpolation::interpolate(t, uff, &timeStamp_, &biasArray_);

  matrix_t k;
  LinearInterpolation::interpolate(indexAlpha, k, &gainArray_);

  uff.noalias() += k * x;
  return uff;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::flatten(const scalar_array_t& timeArray, const std::vector<float_array_t*>& flatArray2) const {
  const auto timeSize = timeArray.size();
  const auto dataSize = flatArray2.size();

  if (timeSize != dataSize) {
    throw std::runtime_error("timeSize and dataSize must be equal in flatten method.");
  }

  for (size_t i = 0; i < timeSize; i++) {
    flattenSingle(timeArray[i], *(flatArray2[i]));
  }
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::flattenSingle(scalar_t time, float_array_t& flatArray) const {
  vector_t uff;
  const auto indexAlpha = LinearInterpolation::interpolate(time, uff, &timeStamp_, &biasArray_);

  matrix_t k;
  LinearInterpolation::interpolate(indexAlpha, k, &gainArray_);

  flatArray.clear();
  flatArray.resize(uff.size() + k.size());

  for (int i = 0; i < k.rows(); i++) {  // i loops through input dim
    flatArray[i * (k.cols() + 1) + 0] = static_cast<float>(uff(i));
    for (int j = 0; j < k.cols(); j++) {  // j loops through state dim
      flatArray[i * (k.cols() + 1) + j + 1] = static_cast<float>(k(i, j));
    }
  }
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::unFlatten(const scalar_array_t& timeArray, const std::vector<float_array_t const*>& flatArray2) {
  if (flatArray2[0]->size() != inputDim_ + inputDim_ * stateDim_) {
    throw std::runtime_error("LinearController::unFlatten received array of wrong length.");
  }

  timeStamp_ = timeArray;

  biasArray_.clear();
  biasArray_.reserve(flatArray2.size());
  gainArray_.clear();
  gainArray_.reserve(flatArray2.size());

  for (const auto& arr : flatArray2) {  // loop through time
    biasArray_.emplace_back(vector_t::Zero(inputDim_));
    gainArray_.emplace_back(matrix_t::Zero(inputDim_, stateDim_));

    for (int i = 0; i < inputDim_; i++) {  // loop through input dim
      biasArray_.back()(i) = static_cast<scalar_t>((*arr)[i * (stateDim_ + 1) + 0]);
      gainArray_.back().row(i) = Eigen::Map<const Eigen::VectorXf>(&((*arr)[i * (stateDim_ + 1) + 1]), stateDim_).cast<scalar_t>();
    }
  }
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::concatenate(const ControllerBase* nextController, int index, int length) {
  if (auto nextLinCtrl = dynamic_cast<const LinearController*>(nextController)) {
    if (!timeStamp_.empty() && timeStamp_.back() > nextLinCtrl->timeStamp_.front()) {
      throw std::runtime_error("Concatenate requires that the nextController comes later in time.");
    }
    int last = index + length;
    timeStamp_.insert(timeStamp_.end(), nextLinCtrl->timeStamp_.begin() + index, nextLinCtrl->timeStamp_.begin() + last);
    biasArray_.insert(biasArray_.end(), nextLinCtrl->biasArray_.begin() + index, nextLinCtrl->biasArray_.begin() + last);
    deltaBiasArray_.insert(deltaBiasArray_.end(), nextLinCtrl->deltaBiasArray_.begin() + index,
                           nextLinCtrl->deltaBiasArray_.begin() + last);
    gainArray_.insert(gainArray_.end(), nextLinCtrl->gainArray_.begin() + index, nextLinCtrl->gainArray_.begin() + last);
  } else {
    throw std::runtime_error("Concatenate only works with controllers of the same type.");
  }
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
int LinearController::size() const {
  return timeStamp_.size();
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
ControllerType LinearController::getType() const {
  return ControllerType::LINEAR;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::clear() {
  timeStamp_.clear();
  biasArray_.clear();
  gainArray_.clear();
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::setZero() {
  // TODO(mspieler): implement this
  throw std::runtime_error("LinearController::setZero() not implemented");

  const size_t inputDim = biasArray_[0].rows();
  const size_t stateDim = gainArray_[0].cols();

  std::fill(biasArray_.begin(), biasArray_.end(), vector_t::Zero(inputDim));
  std::fill(deltaBiasArray_.begin(), deltaBiasArray_.end(), vector_t::Zero(inputDim));
  std::fill(gainArray_.begin(), gainArray_.end(), matrix_t::Zero(inputDim, stateDim));
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
bool LinearController::empty() const {
  return timeStamp_.empty();
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::display() const {
  std::cerr << *this;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::swap(LinearController& other) {
  using std::swap;  // enable ADL

  swap(timeStamp_, other.timeStamp_);
  swap(biasArray_, other.biasArray_);
  swap(deltaBiasArray_, other.deltaBiasArray_);
  swap(gainArray_, other.gainArray_);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::getFeedbackGain(scalar_t time, matrix_t& gain) const {
  LinearInterpolation::interpolate(time, gain, &timeStamp_, &gainArray_);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void LinearController::getBias(scalar_t time, vector_t& bias) const {
  LinearInterpolation::interpolate(time, bias, &timeStamp_, &biasArray_);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
scalar_array_t LinearController::controllerEventTimes() const {
  // simple controller case
  if (timeStamp_.size() < 2) {
    return scalar_array_t(0);
  }

  scalar_array_t eventTimes{0.0};
  scalar_t lastevent = timeStamp_.front();
  for (int i = 0; i < timeStamp_.size() - 1; i++) {
    bool eventDetected = timeStamp_[i + 1] - timeStamp_[i] < 2.0 * OCS2NumericTraits<scalar_t>::weakEpsilon();
    const bool sufficientTimeSinceEvent = timeStamp_[i] - lastevent > 2.0 * OCS2NumericTraits<scalar_t>::weakEpsilon();

    if (eventDetected && sufficientTimeSinceEvent) {  // push back event when event is detected
      eventTimes.push_back(timeStamp_[i]);
      lastevent = eventTimes.back();
    } else if (eventDetected) {
      // if event is detected to close to the last event, it is assumed that the earlier event was not an event
      // but was due to the refining steps taken in event detection
      // The last "detected event" is the time the event took place
      eventTimes.back() = timeStamp_[i];
      lastevent = eventTimes.back();
    }
  }
  return eventTimes;
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void swap(LinearController& a, LinearController& b) {
  a.swap(b);
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
std::ostream& operator<<(std::ostream& out, const LinearController& controller) {
  for (size_t k = 0; k < controller.timeStamp_.size(); k++) {
    out << "k: " << k << '\n';
    out << "time: " << controller.timeStamp_[k] << '\n';
    out << "bias: " << controller.biasArray_[k].transpose() << '\n';
    out << "gain: " << controller.gainArray_[k] << '\n';
  }
}

}  // namespace ocs2
