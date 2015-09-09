// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/extensions/extensions_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/extensions/extension_loader_handler.h"
#include "chrome/browser/ui/webui/extensions/extension_settings_handler.h"
#include "chrome/browser/ui/webui/extensions/install_extension_handler.h"
#include "chrome/browser/ui/webui/metrics_handler.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/browser_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/ownership/owner_settings_service_chromeos_factory.h"
#include "chrome/browser/ui/webui/extensions/chromeos/kiosk_apps_handler.h"
#endif

namespace extensions {

namespace {

content::WebUIDataSource* CreateMdExtensionsSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIExtensionsHost);

  source->SetJsonPath("strings.js");
  source->AddLocalizedString("title",
                             IDS_MANAGE_EXTENSIONS_SETTING_WINDOWS_TITLE);
  source->AddResourcePath("manager.html", IDR_MD_EXTENSIONS_MANAGER_HTML);
  source->AddResourcePath("manager.js", IDR_MD_EXTENSIONS_MANAGER_JS);
  source->AddResourcePath("strings.html", IDR_MD_EXTENSIONS_STRINGS_HTML);
  source->SetDefaultResource(IDR_MD_EXTENSIONS_EXTENSIONS_HTML);

  return source;
}

content::WebUIDataSource* CreateExtensionsHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIExtensionsFrameHost);

  source->SetJsonPath("strings.js");
  source->AddResourcePath("extensions.js", IDR_EXTENSIONS_JS);
  source->AddResourcePath("extension_command_list.js",
                          IDR_EXTENSION_COMMAND_LIST_JS);
  source->AddResourcePath("extension_list.js", IDR_EXTENSION_LIST_JS);
  source->SetDefaultResource(IDR_EXTENSIONS_HTML);
  source->DisableDenyXFrameOptions();
  return source;
}

}  // namespace

ExtensionsUI::ExtensionsUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = nullptr;

  if (::switches::MdExtensionsEnabled()) {
    source = CreateMdExtensionsSource();
  } else {
    source = CreateExtensionsHTMLSource();

    ExtensionSettingsHandler* handler = new ExtensionSettingsHandler();
    handler->GetLocalizedValues(source);
    web_ui->AddMessageHandler(handler);

    ExtensionLoaderHandler* extension_loader_handler =
        new ExtensionLoaderHandler(profile);
    extension_loader_handler->GetLocalizedValues(source);
    web_ui->AddMessageHandler(extension_loader_handler);

    InstallExtensionHandler* install_extension_handler =
        new InstallExtensionHandler();
    install_extension_handler->GetLocalizedValues(source);
    web_ui->AddMessageHandler(install_extension_handler);

#if defined(OS_CHROMEOS)
    chromeos::KioskAppsHandler* kiosk_app_handler =
        new chromeos::KioskAppsHandler(
            chromeos::OwnerSettingsServiceChromeOSFactory::GetForBrowserContext(
                profile));
    kiosk_app_handler->GetLocalizedValues(source);
    web_ui->AddMessageHandler(kiosk_app_handler);
#endif

    web_ui->AddMessageHandler(new MetricsHandler());

    // Need to allow <object> elements so that the <extensionoptions> browser
    // plugin can be loaded within chrome://extensions.
    source->OverrideContentSecurityPolicyObjectSrc("object-src 'self';");
  }

  content::WebUIDataSource::Add(profile, source);
}

ExtensionsUI::~ExtensionsUI() {}

// static
base::RefCountedMemory* ExtensionsUI::GetFaviconResourceBytes(
    ui::ScaleFactor scale_factor) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.LoadDataResourceBytesForScale(IDR_EXTENSIONS_FAVICON, scale_factor);
}

}  // namespace extensions
