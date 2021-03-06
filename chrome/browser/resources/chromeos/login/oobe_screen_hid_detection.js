// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe HID detection screen implementation.
 */

login.createScreen('HIDDetectionScreen', 'hid-detection', function() {
  var CONTEXT_KEY_KEYBOARD_STATE = 'keyboard-state';
  var CONTEXT_KEY_MOUSE_STATE = 'mouse-state';
  var CONTEXT_KEY_KEYBOARD_PINCODE = 'keyboard-pincode';
  var CONTEXT_KEY_KEYBOARD_ENTERED_PART_EXPECTED = 'num-keys-entered-expected';
  var CONTEXT_KEY_KEYBOARD_ENTERED_PART_PINCODE = 'num-keys-entered-pincode';
  var CONTEXT_KEY_MOUSE_DEVICE_NAME = 'mouse-device-name';
  var CONTEXT_KEY_KEYBOARD_DEVICE_NAME = 'keyboard-device-name';
  var CONTEXT_KEY_KEYBOARD_LABEL = 'keyboard-device-label';
  var CONTEXT_KEY_CONTINUE_BUTTON_ENABLED = 'continue-button-enabled';

  var PINCODE_LENGTH = 6;

  return {

  /**
   * Enumeration of possible states during pairing.  The value associated with
   * each state maps to a localized string in the global variable
   * |loadTimeData|.
   * @enum {string}
   */
   PAIRING: {
     STARTUP: 'bluetoothStartConnecting',
     REMOTE_PIN_CODE: 'bluetoothRemotePinCode',
     CONNECT_FAILED: 'bluetoothConnectFailed',
     CANCELED: 'bluetoothPairingCanceled',
     // Pairing dismissed (succeeded or canceled).
     DISMISSED: 'bluetoothPairingDismissed'
   },

   // Enumeration of possible connection states of a device.
   CONNECTION: {
     SEARCHING: 'searching',
     USB: 'usb',
     CONNECTED: 'connected',
     PAIRING: 'pairing',
     PAIRED: 'paired',
     // Special info state.
     UPDATE: 'update'
   },

   // Possible ids of device blocks.
   BLOCK: {
     MOUSE: 'hid-mouse-block',
     KEYBOARD: 'hid-keyboard-block'
   },

    /**
     * Button to move to usual OOBE flow after detection.
     * @private
     */
    continueButton_: null,

    /** @override */
    decorate: function() {
      var self = this;

      this.context.addObserver(
          CONTEXT_KEY_MOUSE_STATE,
          function(stateId) {
            if (stateId === undefined)
              return;
            self.setDeviceBlockState_('hid-mouse-block', stateId);
          }
      );
      this.context.addObserver(
        CONTEXT_KEY_KEYBOARD_STATE,
        function(stateId) {
          self.updatePincodeKeysState_();
          if (stateId === undefined)
            return;
          self.setDeviceBlockState_('hid-keyboard-block', stateId);
          if (stateId == self.CONNECTION.PAIRED) {
            $('hid-keyboard-label-paired').textContent = self.context.get(
                CONTEXT_KEY_KEYBOARD_LABEL, '');
          } else if (stateId == self.CONNECTION.PAIRING) {
            $('hid-keyboard-label-pairing').textContent = self.context.get(
                CONTEXT_KEY_KEYBOARD_LABEL, '');
          }
        }
      );
      this.context.addObserver(
        CONTEXT_KEY_KEYBOARD_PINCODE,
        this.updatePincodeKeysState_.bind(this));
      this.context.addObserver(
        CONTEXT_KEY_KEYBOARD_ENTERED_PART_EXPECTED,
        this.updatePincodeKeysState_.bind(this));
      this.context.addObserver(
        CONTEXT_KEY_KEYBOARD_ENTERED_PART_PINCODE,
        this.updatePincodeKeysState_.bind(this));
      this.context.addObserver(
        CONTEXT_KEY_CONTINUE_BUTTON_ENABLED,
        function(enabled) {
          $('hid-continue-button').disabled = !enabled;
        }
      );
    },

    /**
     * Buttons in oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];
      var continueButton = this.ownerDocument.createElement('button');
      continueButton.id = 'hid-continue-button';
      continueButton.textContent = loadTimeData.getString(
          'hidDetectionContinue');
      continueButton.addEventListener('click', function(e) {
        chrome.send('HIDDetectionOnContinue');
        e.stopPropagation();
      });
      buttons.push(continueButton);

      return buttons;
    },

    /**
     * Returns a control which should receive an initial focus.
     */
    get defaultControl() {
      return $('hid-continue-button');
    },

    /**
     * Sets a device-block css class to reflect device state of searching, usb,
     * connected, pairing or paired (for BT devices).
     * @param {blockId} id one of keys of this.BLOCK dict.
     * @param {state} one of keys of this.CONNECTION dict.
     * @private
     */
    setDeviceBlockState_: function(blockId, state) {
      if (state == 'update')
        return;
      var deviceBlock = $(blockId);
      for (var key in this.CONNECTION) {
        var stateCase = this.CONNECTION[key];
        deviceBlock.classList.toggle(stateCase, stateCase == state);
      }
    },

    /**
     * Sets state for mouse-block.
     * @param {state} one of keys of this.CONNECTION dict.
     */
    setPointingDeviceState: function(state) {
      if (state === undefined)
        return;
      this.setDeviceBlockState_(this.BLOCK.MOUSE, state);
    },

    /**
     * Updates state for pincode key elements based on context state.
     */
    updatePincodeKeysState_: function() {
      var pincodeKeys = $('hid-keyboard-pincode');
      var pincode = this.context.get(CONTEXT_KEY_KEYBOARD_PINCODE, '');
      var state = this.context.get(CONTEXT_KEY_KEYBOARD_STATE, '');

      if (!pincode || state !== this.CONNECTION.PAIRING) {
        pincodeKeys.hidden = true;
        return;
      }

      if (pincodeKeys.hidden) {
        pincodeKeys.hidden = false;
        announceAccessibleMessage(
            this.context.get(CONTEXT_KEY_KEYBOARD_LABEL, '') + ' ' + pincode +
            ' ' + loadTimeData.getString('hidDetectionBTEnterKey'));
      }

      var entered = this.context.get(
          CONTEXT_KEY_KEYBOARD_ENTERED_PART_PINCODE, 0);

      // whether the functionality of getting num of entered keys is available.
      var expected = this.context.get(
          CONTEXT_KEY_KEYBOARD_ENTERED_PART_EXPECTED, false);

      if (pincode.length != PINCODE_LENGTH)
        console.error('Wrong pincode length');

      // Pincode keys plus Enter key.
      for (var i = 0; i < (PINCODE_LENGTH + 1); i++) {
        var pincodeSymbol = $('hid-keyboard-pincode-sym-' + (i + 1));
        pincodeSymbol.classList.toggle('key-typed', i < entered && expected);
        pincodeSymbol.classList.toggle('key-untyped', i > entered && expected);
        pincodeSymbol.classList.toggle('key-next', i == entered && expected);
        if (i < PINCODE_LENGTH)
          pincodeSymbol.textContent = pincode[i] ? pincode[i] : '';
      }
    },

     /*
     * Event handler that is invoked just before the screen in shown.
     * @param {Object} data Screen init payload.
     */
    onBeforeShow: function(data) {
      this.setDeviceBlockState_('hid-mouse-block', this.CONNECTION.SEARCHING);
      this.setDeviceBlockState_('hid-keyboard-block',
                                this.CONNECTION.SEARCHING);
    },
  };
});
