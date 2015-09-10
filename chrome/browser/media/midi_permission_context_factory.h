// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIDI_PERMISSION_CONTEXT_FACTORY_H_
#define CHROME_BROWSER_MEDIA_MIDI_PERMISSION_CONTEXT_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class MidiPermissionContext;
class Profile;

class MidiPermissionContextFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static MidiPermissionContext* GetForProfile(Profile* profile);
  static MidiPermissionContextFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<MidiPermissionContextFactory>;

  MidiPermissionContextFactory();
  ~MidiPermissionContextFactory() override;

  // BrowserContextKeyedBaseFactory methods:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(MidiPermissionContextFactory);
};

#endif  // CHROME_BROWSER_MEDIA_MIDI_PERMISSION_CONTEXT_FACTORY_H_
