// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CUSTOMIZATION_CUSTOMIZATION_DOCUMENT_H_
#define CHROME_BROWSER_CHROMEOS_CUSTOMIZATION_CUSTOMIZATION_DOCUMENT_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class Profile;

namespace base {
class DictionaryValue;
class FilePath;
}

namespace extensions {
class ExternalLoader;
}

namespace net {
class URLFetcher;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chromeos {

class CustomizationWallpaperDownloader;
class ServicesCustomizationExternalLoader;

void InitStartupCustomizationDocumentForTesting(const std::string& manifest);

namespace system {
class StatisticsProvider;
}  // system

// Base class for OEM customization document classes.
class CustomizationDocument {
 public:
  virtual ~CustomizationDocument();

  // Return true if the document was successfully fetched and parsed.
  bool IsReady() const { return root_.get(); }

 protected:
  explicit CustomizationDocument(const std::string& accepted_version);

  virtual bool LoadManifestFromFile(const base::FilePath& manifest_path);
  virtual bool LoadManifestFromString(const std::string& manifest);

  std::string GetLocaleSpecificString(const std::string& locale,
                                      const std::string& dictionary_name,
                                      const std::string& entry_name) const;

  scoped_ptr<base::DictionaryValue> root_;

  // Value of the "version" attribute that is supported.
  // Otherwise config is not loaded.
  std::string accepted_version_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CustomizationDocument);
};

// OEM startup customization document class.
// Now StartupCustomizationDocument is loaded in c-tor so just after create it
// may be ready or not (if manifest is missing or corrupted) and this state
// won't be changed later (i.e. IsReady() always return the same value).
class StartupCustomizationDocument : public CustomizationDocument {
 public:
  static StartupCustomizationDocument* GetInstance();

  std::string GetEULAPage(const std::string& locale) const;

  // These methods can be called even if !IsReady(), in this case VPD values
  // will be returned.
  //
  // Raw value of "initial_locale" like initial_locale="en-US,sv,da,fi,no" .
  const std::string& initial_locale() const { return initial_locale_; }

  // Vector of individual locale values.
  const std::vector<std::string>& configured_locales() const;

  // Default locale value (first value in initial_locale list).
  const std::string& initial_locale_default() const;
  const std::string& initial_timezone() const { return initial_timezone_; }
  const std::string& keyboard_layout() const { return keyboard_layout_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(StartupCustomizationDocumentTest, Basic);
  FRIEND_TEST_ALL_PREFIXES(StartupCustomizationDocumentTest, VPD);
  FRIEND_TEST_ALL_PREFIXES(StartupCustomizationDocumentTest, BadManifest);
  FRIEND_TEST_ALL_PREFIXES(ServicesCustomizationDocumentTest, MultiLanguage);
  friend class OobeLocalizationTest;
  friend void InitStartupCustomizationDocumentForTesting(
      const std::string& manifest);
  friend struct base::DefaultSingletonTraits<StartupCustomizationDocument>;

  // C-tor for singleton construction.
  StartupCustomizationDocument();

  // C-tor for test construction.
  StartupCustomizationDocument(system::StatisticsProvider* provider,
                               const std::string& manifest);

  ~StartupCustomizationDocument() override;

  void Init(system::StatisticsProvider* provider);

  // If |attr| exists in machine stat, assign it to |value|.
  void InitFromMachineStatistic(const char* attr, std::string* value);

  std::string initial_locale_;
  std::vector<std::string> configured_locales_;
  std::string initial_timezone_;
  std::string keyboard_layout_;

  DISALLOW_COPY_AND_ASSIGN(StartupCustomizationDocument);
};

// OEM services customization document class.
// ServicesCustomizationDocument is fetched from network therefore it is not
// ready just after creation. Fetching of the manifest should be initiated
// outside this class by calling StartFetching() or EnsureCustomizationApplied()
// methods.
// User of the file should check IsReady before use it.
class ServicesCustomizationDocument : public CustomizationDocument,
                                      private net::URLFetcherDelegate {
 public:
  static ServicesCustomizationDocument* GetInstance();

  // Registers preferences.
  static void RegisterPrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  static const char kManifestUrl[];

  // Return true if the customization was applied. Customization is applied only
  // once per machine.
  static bool WasOOBECustomizationApplied();

  // If customization has not been applied, start fetching and applying.
  void EnsureCustomizationApplied();

  // Returns Closure with the EnsureCustomizationApplied() method.
  base::Closure EnsureCustomizationAppliedClosure();

  // Start fetching customization document.
  void StartFetching();

  // Apply customization and save in machine options that customization was
  // applied successfully. Return true if customization was applied.
  bool ApplyOOBECustomization();

  // Returns true if default wallpaper URL attribute found in manifest.
  // |out_url| is set to attribute value.
  bool GetDefaultWallpaperUrl(GURL* out_url) const;

  // Returns list of default apps.
  scoped_ptr<base::DictionaryValue> GetDefaultApps() const;

  // Creates an extensions::ExternalLoader that will provide OEM default apps.
  // Cache of OEM default apps stored in profile preferences.
  extensions::ExternalLoader* CreateExternalLoader(Profile* profile);

  // Returns the name of the folder for OEM apps for given |locale|.
  std::string GetOemAppsFolderName(const std::string& locale) const;

  // Initialize instance of ServicesCustomizationDocument for tests that will
  // override singleton until ShutdownForTesting is called.
  static void InitializeForTesting();

  // Remove instance of ServicesCustomizationDocument for tests.
  static void ShutdownForTesting();

  // These methods are also called by WallpaperManager to get "global default"
  // customized wallpaper path (and to init default wallpaper path from it)
  // before first wallpaper is shown.
  static base::FilePath GetCustomizedWallpaperCacheDir();
  static base::FilePath GetCustomizedWallpaperDownloadedFileName();

  CustomizationWallpaperDownloader* wallpaper_downloader_for_testing() {
    return wallpaper_downloader_.get();
  }

 private:
  friend struct base::DefaultSingletonTraits<ServicesCustomizationDocument>;
  FRIEND_TEST_ALL_PREFIXES(CustomizationWallpaperDownloaderBrowserTest,
                           OEMWallpaperIsPresent);
  FRIEND_TEST_ALL_PREFIXES(CustomizationWallpaperDownloaderBrowserTest,
                           OEMWallpaperRetryFetch);

  typedef std::vector<base::WeakPtr<ServicesCustomizationExternalLoader> >
      ExternalLoaders;

  // Guard for a single application task (wallpaper downloading, for example).
  class ApplyingTask;

  // C-tor for singleton construction.
  ServicesCustomizationDocument();

  // C-tor for test construction.
  explicit ServicesCustomizationDocument(const std::string& manifest);

  ~ServicesCustomizationDocument() override;

  // Save applied state in machine settings.
  static void SetApplied(bool val);

  // Overriden from CustomizationDocument:
  bool LoadManifestFromString(const std::string& manifest) override;

  // Overriden from net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Initiate file fetching. Wait for online status.
  void StartFileFetch();

  // Initiate file fetching. Don't wait for online status.
  void DoStartFileFetch();

  // Executes on FILE thread and reads file to string.
  static void ReadFileInBackground(
      base::WeakPtr<ServicesCustomizationDocument> self,
      const base::FilePath& file);

  // Called on UI thread with results of ReadFileInBackground.
  void OnManifesteRead(const std::string& manifest);

  // Method called when manifest was successfully loaded.
  void OnManifestLoaded();

  // Returns list of default apps in ExternalProvider format.
  static scoped_ptr<base::DictionaryValue> GetDefaultAppsInProviderFormat(
      const base::DictionaryValue& root);

  // Update cached manifest for |profile|.
  void UpdateCachedManifest(Profile* profile);

  // Customization document not found for give ID.
  void OnCustomizationNotFound();

  // Set OEM apps folder name for AppListSyncableService for |profile|.
  void SetOemFolderName(Profile* profile, const base::DictionaryValue& root);

  // Returns the name of the folder for OEM apps for given |locale|.
  std::string GetOemAppsFolderNameImpl(
      const std::string& locale,
      const base::DictionaryValue& root) const;

  // Start download of wallpaper image if needed.
  void StartOEMWallpaperDownload(const GURL& wallpaper_url,
                                 scoped_ptr<ApplyingTask> applying);

  // Check that current customized wallpaper cache exists. Once wallpaper is
  // downloaded, it's never updated (even if manifest is re-fetched).
  // Start wallpaper download if needed.
  void CheckAndApplyWallpaper();

  // Intermediate function to pass the result of PathExists to ApplyWallpaper.
  void OnCheckedWallpaperCacheExists(scoped_ptr<bool> exists,
                                     scoped_ptr<ApplyingTask> applying);

  // Called after downloaded wallpaper has been checked.
  void ApplyWallpaper(bool default_wallpaper_file_exists,
                      scoped_ptr<ApplyingTask> applying);

  // Set Shell default wallpaper to customized.
  // It's wrapped as a callback and passed as a parameter to
  // CustomizationWallpaperDownloader.
  void OnOEMWallpaperDownloaded(scoped_ptr<ApplyingTask> applying,
                                bool success,
                                const GURL& wallpaper_url);

  // Register one of Customization applying tasks.
  void ApplyingTaskStarted();

  // Mark task finished and check for "all customization applied".
  void ApplyingTaskFinished(bool success);

  // Services customization manifest URL.
  GURL url_;

  // URLFetcher instance.
  scoped_ptr<net::URLFetcher> url_fetcher_;

  // How many times we already tried to fetch customization manifest file.
  int num_retries_;

  // Manifest fetch is already in progress.
  bool fetch_started_;

  // Delay between checks for network online state.
  base::TimeDelta network_delay_;

  // Known external loaders.
  ExternalLoaders external_loaders_;

  scoped_ptr<CustomizationWallpaperDownloader> wallpaper_downloader_;

  // This is barrier until customization is applied.
  // When number of finished tasks match number of started - customization is
  // applied.
  size_t apply_tasks_started_;
  size_t apply_tasks_finished_;

  // This is the number of successfully finished customization tasks.
  // If it matches number of tasks finished - customization is applied
  // successfully.
  size_t apply_tasks_success_;

  // Weak factory for callbacks.
  base::WeakPtrFactory<ServicesCustomizationDocument> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServicesCustomizationDocument);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CUSTOMIZATION_CUSTOMIZATION_DOCUMENT_H_
