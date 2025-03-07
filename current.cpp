/*
 *   Copyright (c) 2020 Project CHIP Authors
 *   (Heavily modified from the original source(s) that carried this license.)
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#include "commands/example/ExampleCredentialIssuerCommands.h"
#include "commands/icd/ICDCommand.h"
#include <zap-generated/cluster/Commands.h>

class ReadPower : public ReportCommand
{
public:
    ReadPower(chip::ClusterId clusterId, chip::AttributeId attributeId, chip::NodeId destinationId, chip::EndpointId endpointId, CredentialIssuerCommands * credsIssuerConfig) :
        ReportCommand("NA", credsIssuerConfig),
        mClusterIds(1, clusterId),
        mAttributeIds(1, attributeId)
    {
        mEndpointId = std::to_string(endpointId);
        mDestinationId = std::to_string(destinationId);
        mError = CHIP_ERROR_UNINITIALIZED;
    }

    void OnDone(chip::app::ReadClient * aReadClient) override
    {
        InteractionModelReports::CleanupReadClient(aReadClient);
        SetCommandExitStatus(mError);
    }

    void OnAttributeData(const chip::app::ConcreteDataAttributePath & path, chip::TLV::TLVReader * data,
                         const chip::app::StatusIB & status) override
    {
        CHIP_ERROR error = status.ToChipError();
        if (CHIP_NO_ERROR != error)
        {
            LogErrorOnFailure(RemoteDataModelLogger::LogErrorAsJSON(path, status));

            ChipLogError(chipTool, "Response Failure: %s", chip::ErrorStr(error));
            mError = error;
            return;
        }

        if (data == nullptr)
        {
            ChipLogError(chipTool, "Response Failure: No Data");
            mError = CHIP_ERROR_INTERNAL;
            return;
        }

        auto tipe = data->GetType();

        if (tipe == chip::TLV::TLVType::kTLVType_SignedInteger) {
          ChipLogError(chipTool, "It is a signed integer!!");

          LogErrorOnFailure(data->Get(mValue));

           ChipLogError(chipTool, "Its value is: %d", mValue);

        }

        ChipLogError(chipTool, "YES! Response Success!!");
        LogErrorOnFailure(RemoteDataModelLogger::LogAttributeAsJSON(path, data));


        error = DataModelLogger::LogAttribute(path, data);
        if (CHIP_NO_ERROR != error)
        {
            ChipLogError(chipTool, "Response Failure: Can not decode Data");
            mError = error;
            return;
        }

        mError = CHIP_NO_ERROR;
    }
    ~ReadPower() {}

    void SetArgs() {
            char *args[]{mDestinationId.data(), mEndpointId.data()};
            int argc{2};
            AddArguments(false);
            InitArguments(argc, args);
    }

    CHIP_ERROR SendCommand(chip::DeviceProxy * device, std::vector<chip::EndpointId> endpointIds) override
    {
        ChipLogProgress(chipTool, "About to send a read power command");
        return ReportCommand::ReadAttribute(device, endpointIds, mClusterIds, mAttributeIds);
    }

    bool Success() const {
      return mError == CHIP_NO_ERROR;
    }

    int32_t Get() const {
      return mValue;
    }

private:
    std::vector<chip::ClusterId> mClusterIds;
    std::vector<chip::AttributeId> mAttributeIds;
    std::string mDestinationId{};
    std::string mEndpointId{};
    int32_t mValue{};
};

int currentCurrent() {
    ExampleCredentialIssuerCommands credIssuerCommands;
    Commands commands;
    int32_t ac{};
    CHIP_ERROR err = CHIP_NO_ERROR;

    registerCommandsICD(commands, &credIssuerCommands);

    using namespace chip::app::Clusters::ElectricalPowerMeasurement;

    auto readVoltageAttributeCommand = ReadPower(Id,
                    Attributes::ActiveCurrent::Id,
                    0xf, 0x2,
                    &credIssuerCommands);

    if (readVoltageAttributeCommand.Success()) {
      ChipLogProgress(chipTool, "I see success, but should not!");
    }  
    readVoltageAttributeCommand.SetArgs();

    err = chip::Platform::MemoryInit();

    err = readVoltageAttributeCommand.Run();
    VerifyOrExit(err == CHIP_NO_ERROR, ChipLogError(chipTool, "Run command failure: %s", chip::ErrorStr(err)));

    ac = readVoltageAttributeCommand.Get();

    return ac;

exit:
    return -1;
}

