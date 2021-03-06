// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feedback/system_logs/log_sources/crash_ids_source.h"

#include <string>

#include "base/bind.h"
#include "base/time/time.h"
#include "chrome/browser/crash_upload_list/crash_upload_list.h"
#include "components/feedback/feedback_report.h"

namespace system_logs {

namespace {

// The maximum number of crashes we retrieve from the crash list.
constexpr size_t kMaxCrashesCountToRetrieve = 10;

// The length of the crash ID string.
constexpr size_t kCrashIdStringSize = 16;

// We are only interested in crashes that took place within the last hour.
constexpr base::TimeDelta kOneHourTimeDelta = base::TimeDelta::FromHours(1);

}  // namespace

CrashIdsSource::CrashIdsSource()
    : SystemLogsSource("CrashId"),
      crash_upload_list_(CreateCrashUploadList(this)),
      pending_crash_list_loading_(false) {}

CrashIdsSource::~CrashIdsSource() {}

void CrashIdsSource::Fetch(const SysLogsSourceCallback& callback) {
  // Unretained since we own these callbacks.
  pending_requests_.emplace_back(base::Bind(
      &CrashIdsSource::RespondWithCrashIds, base::Unretained(this), callback));

  if (pending_crash_list_loading_)
    return;

  pending_crash_list_loading_ = true;
  crash_upload_list_->LoadUploadListAsynchronously();
}

void CrashIdsSource::OnUploadListAvailable() {
  pending_crash_list_loading_ = false;

  // Only get the IDs of crashes that occurred within the last hour (if any).
  std::vector<UploadList::UploadInfo> crashes;
  crash_upload_list_->GetUploads(kMaxCrashesCountToRetrieve, &crashes);
  const base::Time now = base::Time::Now();
  crash_ids_list_.clear();
  crash_ids_list_.reserve(kMaxCrashesCountToRetrieve *
                          (kCrashIdStringSize + 2));

  // The feedback server expects the crash IDs to be a comma-separated list.
  for (const auto& crash_info : crashes) {
    if (now - crash_info.capture_time < kOneHourTimeDelta) {
      const std::string& crash_id = crash_info.upload_id;
      crash_ids_list_.append(crash_ids_list_.empty() ? crash_id
                                                     : ", " + crash_id);
    }
  }

  for (const auto& request : pending_requests_)
    request.Run();

  pending_requests_.clear();
}

void CrashIdsSource::RespondWithCrashIds(
    const SysLogsSourceCallback& callback) const {
  std::unique_ptr<SystemLogsResponse> response(new SystemLogsResponse());
  (*response)[feedback::FeedbackReport::kCrashReportIdsKey] = crash_ids_list_;

  // We must respond anyways.
  callback.Run(response.get());
}

}  // namespace system_logs
