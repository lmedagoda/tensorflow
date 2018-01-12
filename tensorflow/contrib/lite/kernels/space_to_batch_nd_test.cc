/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <gtest/gtest.h>
#include "tensorflow/contrib/lite/interpreter.h"
#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/kernels/test_util.h"
#include "tensorflow/contrib/lite/model.h"

namespace tflite {
namespace {

using ::testing::ElementsAreArray;

class SpaceToBatchNDOpModel : public SingleOpModel {
 public:
  SpaceToBatchNDOpModel(std::initializer_list<int> input_shape,
                        std::initializer_list<int> block_shape,
                        std::initializer_list<int> before_paddings,
                        std::initializer_list<int> after_paddings) {
    input_ = AddInput(TensorType_FLOAT32);
    output_ = AddOutput(TensorType_FLOAT32);
    SetBuiltinOp(BuiltinOperator_SPACE_TO_BATCH_ND,
                 BuiltinOptions_SpaceToBatchNDOptions,
                 CreateSpaceToBatchNDOptions(
                     builder_, builder_.CreateVector<int>(block_shape),
                     builder_.CreateVector<int>(before_paddings),
                     builder_.CreateVector<int>(after_paddings))
                     .Union());
    BuildInterpreter({input_shape});
  }

  void SetInput(std::initializer_list<float> data) {
    PopulateTensor<float>(input_, data);
  }

  std::vector<float> GetOutput() { return ExtractVector<float>(output_); }
  std::vector<int> GetOutputShape() { return GetTensorShape(output_); }

 private:
  int input_;
  int output_;
};

TEST(SpaceToBatchNDOpTest, InvalidShapeTest) {
  EXPECT_DEATH(SpaceToBatchNDOpModel({1, 3, 3, 1}, {2, 2}, {0, 0}, {0, 0}),
               "Cannot allocate tensors");
}

TEST(SpaceToBatchNDOpTest, SimpleTest) {
  SpaceToBatchNDOpModel m({1, 4, 4, 1}, {2, 2}, {0, 0}, {0, 0});
  m.SetInput({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
  m.Invoke();
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({4, 2, 2, 1}));
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({1, 3, 9, 11, 2, 4, 10, 12, 5, 7,
                                               13, 15, 6, 8, 14, 16}));
}

TEST(SpaceToBatchNDOpTest, MultipleInputBatches) {
  SpaceToBatchNDOpModel m({2, 2, 4, 1}, {2, 2}, {0, 0}, {0, 0});
  m.SetInput({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
  m.Invoke();
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({8, 1, 2, 1}));
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({1, 3, 9, 11, 2, 4, 10, 12, 5, 7,
                                               13, 15, 6, 8, 14, 16}));
}

TEST(SpaceToBatchNDOpTest, SimplePadding) {
  SpaceToBatchNDOpModel m({1, 5, 2, 1}, {3, 2}, {1, 2}, {0, 0});
  m.SetInput({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  m.Invoke();
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({6, 2, 2, 1}));
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({
                                 0, 0, 0, 5, 0, 0, 0, 6, 0, 1, 0, 7,
                                 0, 2, 0, 8, 0, 3, 0, 9, 0, 4, 0, 10,
                             }));
}

TEST(SpaceToBatchNDOpTest, ComplexPadding) {
  SpaceToBatchNDOpModel m({1, 4, 2, 1}, {3, 2}, {1, 2}, {1, 4});
  m.SetInput({1, 2, 3, 4, 5, 6, 7, 8});
  m.Invoke();
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({6, 2, 4, 1}));
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({
                                 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0,
                                 0, 1, 0, 0, 0, 7, 0, 0, 0, 2, 0, 0, 0, 8, 0, 0,
                                 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
                             }));
}

}  // namespace
}  // namespace tflite

int main(int argc, char** argv) {
  ::tflite::LogToStderr();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
