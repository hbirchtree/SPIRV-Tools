// Copyright (c) 2015 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

// Assembler tests for instructions in the "Group Instrucions" section of the
// SPIR-V spec.

#include "UnitSPIRV.h"

#include <cstdint>
#include <limits>

#include "gmock/gmock.h"
#include "TestFixture.h"

namespace {

using spvtest::EnumCase;
using spvtest::MakeInstruction;
using spvtest::Concatenate;
using ::testing::Eq;

// Test Sampler Addressing Mode enum values

using SamplerAddressingModeTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<EnumCase<spv::SamplerAddressingMode>>>;

TEST_P(SamplerAddressingModeTest, AnySamplerAddressingMode) {
  const std::string input =
      "%result = OpConstantSampler %type " + GetParam().name() + " 0 Nearest";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(spv::OpConstantSampler,
                                 {1, 2, GetParam().value(), 0, 0})));
}

// clang-format off
#define CASE(NAME) { spv::SamplerAddressingMode##NAME, #NAME }
INSTANTIATE_TEST_CASE_P(
    TextToBinarySamplerAddressingMode, SamplerAddressingModeTest,
    ::testing::ValuesIn(std::vector<EnumCase<spv::SamplerAddressingMode>>{
        CASE(None),
        CASE(ClampToEdge),
        CASE(Clamp),
        CASE(Repeat),
        CASE(RepeatMirrored),
    }));
#undef CASE
// clang-format on

TEST_F(SamplerAddressingModeTest, WrongMode) {
  EXPECT_THAT(CompileFailure("%r = OpConstantSampler %t xxyyzz 0 Nearest"),
              Eq("Invalid addressing mode 'xxyyzz'."));
}

// Test Sampler Filter Mode enum values

using SamplerFilterModeTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<EnumCase<spv::SamplerFilterMode>>>;

TEST_P(SamplerFilterModeTest, AnySamplerFilterMode) {
  const std::string input =
      "%result = OpConstantSampler %type Clamp 0 " + GetParam().name();
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(spv::OpConstantSampler,
                                 {1, 2, 2, 0, GetParam().value()})));
}

// clang-format off
#define CASE(NAME) { spv::SamplerFilterMode##NAME, #NAME}
INSTANTIATE_TEST_CASE_P(
    TextToBinarySamplerFilterMode, SamplerFilterModeTest,
    ::testing::ValuesIn(std::vector<EnumCase<spv::SamplerFilterMode>>{
        CASE(Nearest),
        CASE(Linear),
    }));
#undef CASE
// clang-format on

TEST_F(SamplerFilterModeTest, WrongMode) {
  EXPECT_THAT(CompileFailure("%r = OpConstantSampler %t Clamp 0 xxyyzz"),
              Eq("Invalid sampler filter mode 'xxyyzz'."));
}

struct ConstantTestCase {
  std::string constant_type;
  std::string constant_value;
  std::vector<uint32_t> expected_instructions;
};

using OpConstantValidTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<ConstantTestCase>>;

TEST_P(OpConstantValidTest, ValidTypes) {
  const std::string input = "%1 = " + GetParam().constant_type +
                            "\n"
                            "%2 = OpConstant %1 " +
                            GetParam().constant_value + "\n";
  std::vector<uint32_t> instructions;
  EXPECT_THAT(CompiledInstructions(input),
              Eq(GetParam().expected_instructions));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    TextToBinaryOpConstantValid, OpConstantValidTest,
    ::testing::ValuesIn(std::vector<ConstantTestCase>{
      // Check 16 bits
      {"OpTypeInt 16 0", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 0}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x1234})})},
      {"OpTypeInt 16 0", "0x8000",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 0}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x8000})})},
      {"OpTypeInt 16 1", "0x8000", // Test sign extension.
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, 0xffff8000})})},
      {"OpTypeInt 16 1", "-32",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, uint32_t(-32)})})},
      // Check 32 bits
      {"OpTypeInt 32 0", "42",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 32, 0}),
         MakeInstruction(spv::OpConstant, {1, 2, 42})})},
      {"OpTypeInt 32 1", "-32",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 32, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, uint32_t(-32)})})},
      {"OpTypeFloat 32", "1.0",
        Concatenate({MakeInstruction(spv::OpTypeFloat, {1, 32}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x3f800000})})},
      {"OpTypeFloat 32", "10.0",
        Concatenate({MakeInstruction(spv::OpTypeFloat, {1, 32}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x41200000})})},
      // Check 48 bits
      {"OpTypeInt 48 0", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 0}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x1234, 0})})},
      {"OpTypeInt 48 0", "0x800000000001",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 0}),
         MakeInstruction(spv::OpConstant, {1, 2, 1, 0x00008000})})},
      {"OpTypeInt 48 1", "0x800000000000", // Test sign extension.
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, 0, 0xffff8000})})},
      {"OpTypeInt 48 1", "-32",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, uint32_t(-32), uint32_t(-1)})})},
      // Check 64 bits
      {"OpTypeInt 64 0", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 64, 0}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x1234, 0})})},
      {"OpTypeInt 64 1", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 64, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, 0x1234, 0})})},
      {"OpTypeInt 64 1", "-42",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 64, 1}),
         MakeInstruction(spv::OpConstant, {1, 2, uint32_t(-42), uint32_t(-1)})})},
    }));
// clang-format on

using OpConstantInvalidTypeTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<std::string>>;

TEST_P(OpConstantInvalidTypeTest, InvalidTypes) {
  const std::string input = "%1 = " + GetParam() +
                            "\n"
                            "%2 = OpConstant %1 0\n";
  EXPECT_THAT(
      CompileFailure(input),
      Eq("Type for Constant must be a scalar floating point or integer type"));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    TextToBinaryOpConstantInvalidValidType, OpConstantInvalidTypeTest,
    ::testing::ValuesIn(std::vector<std::string>{
      {"OpTypeVoid",
       "OpTypeBool",
       "OpTypeVector %a 32",
       "OpTypeMatrix %a 32",
       "OpTypeImage %a 1D 0 0 0 0 Unknown",
       "OpTypeSampler",
       "OpTypeSampledImage %a",
       "OpTypeArray %a %b",
       "OpTypeRuntimeArray %a",
       "OpTypeStruct %a",
       "OpTypeOpaque \"Foo\"",
       "OpTypePointer UniformConstant %a",
       "OpTypeFunction %a %b",
       "OpTypeEvent",
       "OpTypeDeviceEvent",
       "OpTypeReserveId",
       "OpTypeQueue",
       "OpTypePipe ReadOnly",
       "OpTypeForwardPointer %a UniformConstant",
        // At least one thing that isn't a type at all
       "OpNot %a %b"
      },
    }));
// clang-format on

using OpSpecConstantValidTest = spvtest::TextToBinaryTestBase<
  ::testing::TestWithParam<ConstantTestCase>>;

TEST_P(OpSpecConstantValidTest, ValidTypes)
{
  const std::string input = "%1 = " + GetParam().constant_type +
                            "\n"
                            "%2 = OpSpecConstant %1 " +
                            GetParam().constant_value + "\n";
  std::vector<uint32_t> instructions;
  EXPECT_THAT(CompiledInstructions(input),
              Eq(GetParam().expected_instructions));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    TextToBinaryOpSpecConstantValid, OpSpecConstantValidTest,
    ::testing::ValuesIn(std::vector<ConstantTestCase>{
      // Check 16 bits
      {"OpTypeInt 16 0", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 0}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x1234})})},
      {"OpTypeInt 16 0", "0x8000",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 0}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x8000})})},
      {"OpTypeInt 16 1", "0x8000", // Test sign extension.
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0xffff8000})})},
      {"OpTypeInt 16 1", "-32",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 16, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, uint32_t(-32)})})},
      // Check 32 bits
      {"OpTypeInt 32 0", "42",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 32, 0}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 42})})},
      {"OpTypeInt 32 1", "-32",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 32, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, uint32_t(-32)})})},
      {"OpTypeFloat 32", "1.0",
        Concatenate({MakeInstruction(spv::OpTypeFloat, {1, 32}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x3f800000})})},
      {"OpTypeFloat 32", "10.0",
        Concatenate({MakeInstruction(spv::OpTypeFloat, {1, 32}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x41200000})})},
      // Check 48 bits
      {"OpTypeInt 48 0", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 0}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x1234, 0})})},
      {"OpTypeInt 48 0", "0x800000000001",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 0}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 1, 0x00008000})})},
      {"OpTypeInt 48 1", "0x800000000000", // Test sign extension.
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0, 0xffff8000})})},
      {"OpTypeInt 48 1", "-32",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 48, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, uint32_t(-32), uint32_t(-1)})})},
      // Check 64 bits
      {"OpTypeInt 64 0", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 64, 0}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x1234, 0})})},
      {"OpTypeInt 64 1", "0x1234",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 64, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, 0x1234, 0})})},
      {"OpTypeInt 64 1", "-42",
        Concatenate({MakeInstruction(spv::OpTypeInt, {1, 64, 1}),
         MakeInstruction(spv::OpSpecConstant, {1, 2, uint32_t(-42), uint32_t(-1)})})},
    }));
// clang-format on

using OpSpecConstantInvalidTypeTest = spvtest::TextToBinaryTestBase<
  ::testing::TestWithParam<std::string>>;

TEST_P(OpSpecConstantInvalidTypeTest, InvalidTypes)
{
  const std::string input = "%1 = " + GetParam() +
                            "\n"
                            "%2 = OpSpecConstant %1 0\n";
  EXPECT_THAT(
      CompileFailure(input),
      Eq("Type for SpecConstant must be a scalar floating point or integer type"));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(
    TextToBinaryOpSpecConstantInvalidValidType, OpSpecConstantInvalidTypeTest,
    ::testing::ValuesIn(std::vector<std::string>{
      {"OpTypeVoid",
       "OpTypeBool",
       "OpTypeVector %a 32",
       "OpTypeMatrix %a 32",
       "OpTypeImage %a 1D 0 0 0 0 Unknown",
       "OpTypeSampler",
       "OpTypeSampledImage %a",
       "OpTypeArray %a %b",
       "OpTypeRuntimeArray %a",
       "OpTypeStruct %a",
       "OpTypeOpaque \"Foo\"",
       "OpTypePointer UniformConstant %a",
       "OpTypeFunction %a %b",
       "OpTypeEvent",
       "OpTypeDeviceEvent",
       "OpTypeReserveId",
       "OpTypeQueue",
       "OpTypePipe ReadOnly",
       "OpTypeForwardPointer %a UniformConstant",
        // At least one thing that isn't a type at all
       "OpNot %a %b"
      },
    }));
// clang-format on

using RoundTripTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<std::string>>;

const int64_t kMaxUnsigned48Bit = (int64_t(1) << 48) - 1;
const int64_t kMaxSigned48Bit = (int64_t(1) << 47) - 1;
const int64_t kMinSigned48Bit = -kMaxSigned48Bit - 1;

TEST_P(RoundTripTest, Sample) {
  EXPECT_THAT(EncodeAndDecodeSuccessfully(GetParam()),
              Eq(GetParam()));
}

// TODO(dneto): Enable support once this works.
INSTANTIATE_TEST_CASE_P(DISABLED_OpConstantRoundTrip, RoundTripTest,
    ::testing::ValuesIn(std::vector<std::string>{
        // 16 bit
        "%1 = OpTypeInt 16 0\n%2 = OpConstant %1 0\n",
        "%1 = OpTypeInt 16 0\n%2 = OpConstant %1 65535\n",
        "%1 = OpTypeInt 16 1\n%2 = OpConstant %1 -32768\n",
        "%1 = OpTypeInt 16 1\n%2 = OpConstant %1 32767\n",
        "%1 = OpTypeInt 32 0\n%2 = OpConstant %1 0\n",
        // 32 bit
        std::string("%1 = OpTypeInt 32 0\n%2 = OpConstant %1 0\n"),
        std::string("%1 = OpTypeInt 32 0\n%2 = OpConstant %1 ") +
            std::to_string(std::numeric_limits<uint32_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 32 1\n%2 = OpConstant %1 ") +
            std::to_string(std::numeric_limits<int32_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 32 1\n%2 = OpConstant %1 ") +
            std::to_string(std::numeric_limits<int32_t>::min()) + "\n",
        // 48 bit
        std::string("%1 = OpTypeInt 48 0\n%2 = OpConstant %1 0\n"),
        std::string("%1 = OpTypeInt 48 0\n%2 = OpConstant %1 ") +
            std::to_string(kMaxUnsigned48Bit) + "\n",
        std::string("%1 = OpTypeInt 48 1\n%2 = OpConstant %1 ") +
            std::to_string(kMaxSigned48Bit) + "\n",
        std::string("%1 = OpTypeInt 48 1\n%2 = OpConstant %1 ") +
            std::to_string(kMinSigned48Bit) + "\n",
        // 64 bit
        std::string("%1 = OpTypeInt 64 0\n%2 = OpConstant %1 0\n"),
        std::string("%1 = OpTypeInt 64 0\n%2 = OpConstant %1 ") +
            std::to_string(std::numeric_limits<uint64_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 64 1\n%2 = OpConstant %1 ") +
            std::to_string(std::numeric_limits<int64_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 64 1\n%2 = OpConstant %1 ") +
            std::to_string(std::numeric_limits<int64_t>::min()) + "\n",
        // 32-bit float
        "%1 = OpTypeFloat 32\n%2 = OpConstant %1 0\n",
        "%1 = OpTypeFloat 32\n%2 = OpConstant %1 13.5\n",
        "%1 = OpTypeFloat 32\n%2 = OpConstant %1 -12.5\n",
        // 64-bit float
        "%1 = OpTypeFloat 64\n%2 = OpConstant %1 0\n",
        "%1 = OpTypeFloat 64\n%2 = OpConstant %1 1.79769e+308\n",
        "%1 = OpTypeFloat 64\n%2 = OpConstant %1 -1.79769e+308\n",
    }));

// TODO(dneto): Enable support once this works.
INSTANTIATE_TEST_CASE_P(DISABLED_OpSpecConstantRoundTrip, RoundTripTest,
    ::testing::ValuesIn(std::vector<std::string>{
        // 16 bit
        "%1 = OpTypeInt 16 0\n%2 = OpSpecConstant %1 0\n",
        "%1 = OpTypeInt 16 0\n%2 = OpSpecConstant %1 65535\n",
        "%1 = OpTypeInt 16 1\n%2 = OpSpecConstant %1 -32768\n",
        "%1 = OpTypeInt 16 1\n%2 = OpSpecConstant %1 32767\n",
        "%1 = OpTypeInt 32 0\n%2 = OpSpecConstant %1 0\n",
        // 32 bit
        std::string("%1 = OpTypeInt 32 0\n%2 = OpSpecConstant %1 0\n"),
        std::string("%1 = OpTypeInt 32 0\n%2 = OpSpecConstant %1 ") +
            std::to_string(std::numeric_limits<uint32_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 32 1\n%2 = OpSpecConstant %1 ") +
            std::to_string(std::numeric_limits<int32_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 32 1\n%2 = OpSpecConstant %1 ") +
            std::to_string(std::numeric_limits<int32_t>::min()) + "\n",
        // 48 bit
        std::string("%1 = OpTypeInt 48 0\n%2 = OpSpecConstant %1 0\n"),
        std::string("%1 = OpTypeInt 48 0\n%2 = OpSpecConstant %1 ") +
            std::to_string(kMaxUnsigned48Bit) + "\n",
        std::string("%1 = OpTypeInt 48 1\n%2 = OpSpecConstant %1 ") +
            std::to_string(kMaxSigned48Bit) + "\n",
        std::string("%1 = OpTypeInt 48 1\n%2 = OpSpecConstant %1 ") +
            std::to_string(kMinSigned48Bit) + "\n",
        // 64 bit
        std::string("%1 = OpTypeInt 64 0\n%2 = OpSpecConstant %1 0\n"),
        std::string("%1 = OpTypeInt 64 0\n%2 = OpSpecConstant %1 ") +
            std::to_string(std::numeric_limits<uint64_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 64 1\n%2 = OpSpecConstant %1 ") +
            std::to_string(std::numeric_limits<int64_t>::max()) + "\n",
        std::string("%1 = OpTypeInt 64 1\n%2 = OpSpecConstant %1 ") +
            std::to_string(std::numeric_limits<int64_t>::min()) + "\n",
        // 32-bit float
        "%1 = OpTypeFloat 32\n%2 = OpSpecConstant %1 0\n",
        "%1 = OpTypeFloat 32\n%2 = OpSpecConstant %1 13.5\n",
        "%1 = OpTypeFloat 32\n%2 = OpSpecConstant %1 -12.5\n",
        // 64-bit float
        "%1 = OpTypeFloat 64\n%2 = OpSpecConstant %1 0\n",
        "%1 = OpTypeFloat 64\n%2 = OpSpecConstant %1 1.79769e+308\n",
        "%1 = OpTypeFloat 64\n%2 = OpSpecConstant %1 -1.79769e+308\n",
    }));

// TODO(dneto): OpConstantTrue
// TODO(dneto): OpConstantFalse
// TODO(dneto): OpConstantComposite
// TODO(dneto): OpConstantSampler: other variations Param is 0 or 1
// TODO(dneto): OpConstantNull
// TODO(dneto): OpSpecConstantTrue
// TODO(dneto): OpSpecConstantFalse
// TODO(dneto): OpSpecConstantComposite
// TODO(dneto): OpSpecConstantOp

}  // anonymous namespace