/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parser_util.h"

#include <cstdio>
#include <cstring>

#include <climits>

#include "preferences_util.h"
#include "data_storage_log_wrapper.h"


namespace OHOS {
namespace Telephony {
int ParserUtil::ParserPdpProfileJson(std::vector<PdpProfile> &vec)
{
    char *content;
    int ret = LoaderJsonFile(content);
    if (ret != DATA_STORAGE_SUCCESS) {
        DATA_STORAGE_LOGE("ParserUtil::ParserPdpProfileJson LoaderJsonFile is fail!\n");
        return ret;
    }
    const int contentLength = strlen(content);
    const std::string rawJson(content);
    JSONCPP_STRING err;
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader(builder.newCharReader());
    if (!reader->parse(rawJson.c_str(), rawJson.c_str() + contentLength, &root, &err)) {
        DATA_STORAGE_LOGE("ParserUtil::ParserPdpProfileJson reader is error!\n");
        return static_cast<int>(LoadProFileErrorType::FILE_PARSER_ERROR);
    }
    delete reader;
    const int apnVersion = root[ITEM_VERSION].asInt();
    PreferencesUtil *utils = DelayedSingleton<PreferencesUtil>::GetInstance().get();
    if (utils == nullptr) {
        DATA_STORAGE_LOGE("ParserUtil::ParserPdpProfileJson utils is nullptr!\n");
        return static_cast<int>(LoadProFileErrorType::TEL_PROFILE_UTIL_IS_NULL);
    }
    int profileVersion = utils->ObtainInt(APN_VERSION, 0);
    if (apnVersion <= profileVersion) {
        DATA_STORAGE_LOGE("ParserUtil::ParserPdpProfileJson apnVersion <= profileVersion!\n");
        return static_cast<int>(LoadProFileErrorType::PDP_PROFILE_VERSION_IS_OLD);
    }
    Json::Value itemRoots = root[ITEM_OPERATOR_INFOS];
    if (itemRoots.size() == 0) {
        DATA_STORAGE_LOGE("ParserUtil::ParserPdpProfileJson itemRoots size == 0!\n");
        return static_cast<int>(LoadProFileErrorType::ITEM_SIZE_IS_NULL);
    }
    ParserPdpProfileInfos(vec, itemRoots);
    ret = utils->SaveInt(APN_VERSION, apnVersion);
    if (ret == NativePreferences::E_OK) {
        utils->Refresh();
        ret = DATA_STORAGE_SUCCESS;
    }
    DATA_STORAGE_LOGI("ParserUtil::ParserPdpProfileJson##apnVersion = %{public}d\n", apnVersion);
    return ret;
}

void ParserUtil::ParserPdpProfileInfos(std::vector<PdpProfile> &vec, Json::Value &root)
{
    for (int i = 0; i < root.size(); i++) {
        Json::Value itemRoot = root[i];
        PdpProfile bean;
        bean.profileName = itemRoot[ITEM_OPERATOR_NAME].asString();
        bean.authUser = itemRoot[ITEM_AUTH_USER].asString();
        bean.authPwd = itemRoot[ITEM_AUTH_PWD].asString();
        std::string authTypeStr = itemRoot[ITEM_AUTH_TYPE].asString();
        if (authTypeStr.empty()) {
            bean.authType = 0;
        } else {
            bean.authType = atoi(authTypeStr.c_str());
        }
        bean.mcc = itemRoot[ITEM_MCC].asString();
        bean.mnc = itemRoot[ITEM_MNC].asString();
        bean.apn = itemRoot[ITEM_APN].asString();
        bean.apnTypes = itemRoot[ITEM_APN_TYPES].asString();
        bean.mmsIpAddress = itemRoot[ITEM_MMS_IP_ADDRESS].asString();
        bean.proxyIpAddress =  itemRoot[ITEM_IP_ADDRESS].asString();
        bean.homeUrl =  itemRoot[ITEM_HOME_URL].asString();
        vec.push_back(bean);
    }
}

void ParserUtil::ParserPdpProfileToValuesBucket(NativeRdb::ValuesBucket &value, const PdpProfile &bean)
{
    value.PutString(PdpProfileData::PROFILE_NAME, bean.profileName);
    value.PutString(PdpProfileData::MCC, bean.mcc);
    value.PutString(PdpProfileData::MNC, bean.mnc);
    std::string mccmnc(bean.mcc);
    mccmnc.append(bean.mnc);
    value.PutString(PdpProfileData::MCCMNC, mccmnc);
    value.PutString(PdpProfileData::APN, bean.apn);
    value.PutInt(PdpProfileData::AUTH_TYPE, bean.authType);
    value.PutString(PdpProfileData::AUTH_USER, bean.authUser);
    value.PutString(PdpProfileData::AUTH_PWD, bean.authPwd);
    value.PutString(PdpProfileData::APN_TYPES, bean.apnTypes);
    value.PutBool(PdpProfileData::IS_ROAMING_APN, bean.isRoamingApn);
    value.PutString(PdpProfileData::HOME_URL, bean.homeUrl);
    value.PutString(PdpProfileData::PROXY_IP_ADDRESS, bean.proxyIpAddress);
    value.PutString(PdpProfileData::MMS_IP_ADDRESS, bean.mmsIpAddress);
    value.PutString(PdpProfileData::APN_PROTOCOL, bean.pdpProtocol);
    value.PutString(PdpProfileData::APN_ROAM_PROTOCOL, bean.roamPdpProtocol);
}

int ParserUtil::LoaderJsonFile(char *&content) const
{
    size_t len = 0;
    FILE *f = fopen(PATH, "rb");
    if (f == nullptr) {
        DATA_STORAGE_LOGE("ParserUtil::LoaderJsonFile file is null!\n");
        return static_cast<int>(LoadProFileErrorType::OPEN_FILE_ERROR);
    }
    int ret_seek_end = fseek(f, 0, SEEK_END);
    if (ret_seek_end != 0) {
        DATA_STORAGE_LOGE("ParserUtil::LoaderJsonFile ret_seek_end != 0!\n");
        CloseFile(f);
        return static_cast<int>(LoadProFileErrorType::LOAD_FILE_ERROR);
    }
    len = ftell(f);
    int ret_seek_set = fseek(f, 0, SEEK_SET);
    if (ret_seek_set != 0) {
        DATA_STORAGE_LOGE("ParserUtil::LoaderJsonFile ret_seek_set != 0!\n");
        CloseFile(f);
        return static_cast<int>(LoadProFileErrorType::LOAD_FILE_ERROR);
    }
    if (len <= 0 || len > ULONG_MAX) {
        DATA_STORAGE_LOGE("ParserUtil::LoaderJsonFile len <= 0 or len > LONG_MAX!\n");
        CloseFile(f);
        return static_cast<int>(LoadProFileErrorType::LOAD_FILE_ERROR);
    }
    content = (char *)malloc(len);
    size_t ret_read = fread(content, 1, len, f);
    if (ret_read != len) {
        DATA_STORAGE_LOGE("ParserUtil::LoaderJsonFile ret_read != len!\n");
        CloseFile(f);
        return static_cast<int>(LoadProFileErrorType::LOAD_FILE_ERROR);
    }
    return CloseFile(f);
}

int ParserUtil::CloseFile(FILE *f) const
{
    int ret_close = fclose(f);
    free(f);
    f = nullptr;
    if (ret_close != 0) {
        DATA_STORAGE_LOGE("ParserUtil::LoaderJsonFile ret_close != 0!\n");
        return static_cast<int>(LoadProFileErrorType::CLOSE_FILE_ERROR);
    }
    return DATA_STORAGE_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS