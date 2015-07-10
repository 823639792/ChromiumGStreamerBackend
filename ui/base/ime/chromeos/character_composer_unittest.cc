// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/chromeos/character_composer.h"

#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/keyboard_codes.h"

using base::ASCIIToUTF16;

namespace ui {

namespace {

const base::char16 kCombiningGrave = 0x0300;
const base::char16 kCombiningAcute = 0x0301;
const base::char16 kCombiningCircumflex = 0x0302;
const base::char16 kCombiningHorn = 0x031B;

}  // namespace

class CharacterComposerTest : public testing::Test {
 protected:
  // Returns a |KeyEvent| for a dead key press.
  KeyEvent* DeadKeyPress(base::char16 combining_character) const {
    KeyEvent* event =
        new KeyEvent(ET_KEY_PRESSED, VKEY_UNKNOWN, DomCode::NONE, EF_NONE,
                     DomKey::DEAD, combining_character, EventTimeForNow());
    return event;
  }

  // Expects key is filtered and no character is composed.
  void ExpectDeadKeyFiltered(base::char16 combining_character) {
    scoped_ptr<KeyEvent> event(DeadKeyPress(combining_character));
    EXPECT_TRUE(character_composer_.FilterKeyPress(*event));
    EXPECT_TRUE(character_composer_.composed_character().empty());
  }

  // Expects key is filtered and the given character is composed.
  void ExpectDeadKeyComposed(base::char16 combining_character,
                             const base::string16& expected_character) {
    scoped_ptr<KeyEvent> event(DeadKeyPress(combining_character));
    EXPECT_TRUE(character_composer_.FilterKeyPress(*event));
    EXPECT_EQ(expected_character, character_composer_.composed_character());
  }

  // Returns a |KeyEvent| for a character key press.
  KeyEvent* UnicodeKeyPress(KeyboardCode vkey,
                            DomCode code,
                            int flags,
                            base::char16 character) const {
    KeyEvent* event = new KeyEvent(ET_KEY_PRESSED, vkey, code, flags,
                                   CharacterToDomKey(character), character,
                                   EventTimeForNow());
    return event;
  }

  // Expects key is not filtered and no character is composed.
  void ExpectUnicodeKeyNotFiltered(KeyboardCode vkey,
                                   DomCode code,
                                   int flags,
                                   base::char16 character) {
    scoped_ptr<KeyEvent> event(UnicodeKeyPress(vkey, code, flags, character));
    EXPECT_FALSE(character_composer_.FilterKeyPress(*event));
    EXPECT_TRUE(character_composer_.composed_character().empty());
  }

  // Expects key is filtered and no character is composed.
  void ExpectUnicodeKeyFiltered(KeyboardCode vkey,
                                DomCode code,
                                int flags,
                                base::char16 character) {
    scoped_ptr<KeyEvent> event(UnicodeKeyPress(vkey, code, flags, character));
    EXPECT_TRUE(character_composer_.FilterKeyPress(*event));
    EXPECT_TRUE(character_composer_.composed_character().empty());
  }

  // Expects key is filtered and the given character is composed.
  void ExpectUnicodeKeyComposed(KeyboardCode vkey,
                                DomCode code,
                                int flags,
                                base::char16 character,
                                const base::string16& expected_character) {
    scoped_ptr<KeyEvent> event(UnicodeKeyPress(vkey, code, flags, character));
    EXPECT_TRUE(character_composer_.FilterKeyPress(*event));
    EXPECT_EQ(expected_character, character_composer_.composed_character());
  }

  CharacterComposer character_composer_;
};

TEST_F(CharacterComposerTest, InitialState) {
  EXPECT_TRUE(character_composer_.composed_character().empty());
}

TEST_F(CharacterComposerTest, NormalKeyIsNotFiltered) {
  ExpectUnicodeKeyNotFiltered(VKEY_B, DomCode::KEY_B, EF_NONE, 'B');
  ExpectUnicodeKeyNotFiltered(VKEY_Z, DomCode::KEY_Z, EF_NONE, 'Z');
  ExpectUnicodeKeyNotFiltered(VKEY_C, DomCode::KEY_C, EF_NONE, 'c');
  ExpectUnicodeKeyNotFiltered(VKEY_M, DomCode::KEY_M, EF_NONE, 'm');
  ExpectUnicodeKeyNotFiltered(VKEY_0, DomCode::DIGIT0, EF_NONE, '0');
  ExpectUnicodeKeyNotFiltered(VKEY_1, DomCode::DIGIT1, EF_NONE, '1');
  ExpectUnicodeKeyNotFiltered(VKEY_8, DomCode::DIGIT8, EF_NONE, '8');
}

TEST_F(CharacterComposerTest, PartiallyMatchingSequence) {
  // Composition with sequence ['dead acute', '1'] will fail.
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');

  // Composition with sequence ['dead acute', 'dead circumflex', '1'] will fail.
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectDeadKeyFiltered(kCombiningCircumflex);
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');
}

TEST_F(CharacterComposerTest, FullyMatchingSequences) {
  // LATIN SMALL LETTER A WITH ACUTE
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'a',
                           base::string16(1, 0x00E1));
  // LATIN CAPITAL LETTER A WITH ACUTE
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'A',
                           base::string16(1, 0x00C1));
  // GRAVE ACCENT
  ExpectDeadKeyFiltered(kCombiningGrave);
  ExpectDeadKeyComposed(kCombiningGrave, base::string16(1, 0x0060));
  // LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectDeadKeyFiltered(kCombiningCircumflex);
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'a',
                           base::string16(1, 0x1EA5));
  // LATIN CAPITAL LETTER U WITH HORN AND GRAVE
  ExpectDeadKeyFiltered(kCombiningGrave);
  ExpectDeadKeyFiltered(kCombiningHorn);
  ExpectUnicodeKeyComposed(VKEY_U, DomCode::KEY_U, EF_NONE, 'U',
                           base::string16(1, 0x1EEA));
  // LATIN CAPITAL LETTER C WITH CEDILLA
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyComposed(VKEY_C, DomCode::KEY_C, EF_NONE, 'C',
                           base::string16(1, 0x00C7));
  // LATIN SMALL LETTER C WITH CEDILLA
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyComposed(VKEY_C, DomCode::KEY_C, EF_NONE, 'c',
                           base::string16(1, 0x00E7));
}

TEST_F(CharacterComposerTest, FullyMatchingSequencesAfterMatchingFailure) {
  // Composition with sequence ['dead acute', 'dead circumflex', '1'] will fail.
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectDeadKeyFiltered(kCombiningCircumflex);
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');
  // LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectDeadKeyFiltered(kCombiningCircumflex);
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'a',
                           base::string16(1, 0x1EA5));
}

TEST_F(CharacterComposerTest, ComposedCharacterIsClearedAfterReset) {
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'a',
                           base::string16(1, 0x00E1));
  character_composer_.Reset();
  EXPECT_TRUE(character_composer_.composed_character().empty());
}

TEST_F(CharacterComposerTest, CompositionStateIsClearedAfterReset) {
  // Even though sequence ['dead acute', 'a'] will compose 'a with acute',
  // no character is composed here because of reset.
  ExpectDeadKeyFiltered(kCombiningAcute);
  character_composer_.Reset();
  ExpectUnicodeKeyNotFiltered(VKEY_A, DomCode::KEY_A, EF_NONE, 'a');
}

TEST_F(CharacterComposerTest, KeySequenceCompositionPreedit) {
  // LATIN SMALL LETTER A WITH ACUTE
  // preedit_string() is always empty in key sequence composition mode.
  ExpectDeadKeyFiltered(kCombiningAcute);
  EXPECT_TRUE(character_composer_.preedit_string().empty());
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'a',
                           base::string16(1, 0x00E1));
  EXPECT_TRUE(character_composer_.preedit_string().empty());
}

// Verify the structure of the primary |TreeComposeChecker| table.
TEST_F(CharacterComposerTest, MainTableIsCorrectlyOrdered) {
// This file is included here intentionally, instead of the top of the file,
// because including this file at the top of the file will define a
// global constant and contaminate the global namespace.
#include "ui/base/ime/chromeos/character_composer_data.h"
  const int kTypes = 2;

  // Record the subtree locations and check subtable sizes.
  std::vector<uint16_t> subtrees;
  uint16_t index = 0;
  while (index < kCompositions.tree_entries) {
    // Record the start of the subtree.
    SCOPED_TRACE(index);
    subtrees.push_back(index);
    for (int t = 0; t < kTypes; ++t) {
      // Skip the internal table and verify the next index is within the data.
      index += 1 + 2 * kCompositions.tree[index];
      EXPECT_GT(kCompositions.tree_entries, index);
      // Skip the leaf table and verify that the next index is not past the
      // end of the data.
      index += 1 + 2 * kCompositions.tree[index];
      EXPECT_GE(kCompositions.tree_entries, index);
    }
  }
  // We should end up at the end of the data.
  EXPECT_EQ(kCompositions.tree_entries, index);

  // Check subtable structure.
  index = 0;
  while (index < kCompositions.tree_entries) {
    SCOPED_TRACE(index);
    for (int t = 0; t < kTypes; ++t) {
      // Check the internal subtable.
      uint16_t previous_key = 0;
      uint16_t size = kCompositions.tree[index++];
      for (uint16_t i = 0; i < size; ++i) {
        // Verify that the subtable is sorted.
        uint16_t key = kCompositions.tree[index];
        uint16_t value = kCompositions.tree[index + 1];
        if (i)
          EXPECT_LT(previous_key, key) << index;
        previous_key = key;
        // Verify that the internal link is valid.
        const auto it = std::find(subtrees.begin(), subtrees.end(), value);
        EXPECT_FALSE(subtrees.end() == it) << index;
        index += 2;
      }
      // Check the leaf subtable.
      previous_key = 0;
      size = kCompositions.tree[index++];
      for (uint16_t i = 0; i < size; ++i) {
        // Verify that the subtable is sorted.
        uint16_t key = kCompositions.tree[index];
        if (i)
          EXPECT_LT(previous_key, key) << index;
        previous_key = key;
        index += 2;
      }
    }
  }
}

TEST_F(CharacterComposerTest, HexadecimalComposition) {
  // HIRAGANA LETTER A (U+3042)
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 'U');
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, EF_NONE, '0');
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, EF_NONE, '4');
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ',
                           base::string16(1, 0x3042));
  // MUSICAL KEYBOARD (U+1F3B9)
  const base::char16 kMusicalKeyboard[] = {0xd83c, 0xdfb9};
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 'U');
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, EF_NONE, '1');
  ExpectUnicodeKeyFiltered(VKEY_F, DomCode::KEY_F, EF_NONE, 'f');
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
  ExpectUnicodeKeyFiltered(VKEY_B, DomCode::KEY_B, EF_NONE, 'b');
  ExpectUnicodeKeyFiltered(VKEY_9, DomCode::DIGIT9, EF_NONE, '9');
  ExpectUnicodeKeyComposed(
      VKEY_RETURN, DomCode::ENTER, EF_NONE, '\r',
      base::string16(kMusicalKeyboard,
                     kMusicalKeyboard + arraysize(kMusicalKeyboard)));
}

TEST_F(CharacterComposerTest, HexadecimalCompositionPreedit) {
  // HIRAGANA LETTER A (U+3042)
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 'U');
  EXPECT_EQ(ASCIIToUTF16("u"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, 0, '3');
  EXPECT_EQ(ASCIIToUTF16("u3"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, 0, '0');
  EXPECT_EQ(ASCIIToUTF16("u30"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, 0, '4');
  EXPECT_EQ(ASCIIToUTF16("u304"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_A, DomCode::KEY_A, 0, 'a');
  EXPECT_EQ(ASCIIToUTF16("u304a"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_BACK, DomCode::BACKSPACE, EF_NONE, '\b');
  EXPECT_EQ(ASCIIToUTF16("u304"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_RETURN, DomCode::ENTER, EF_NONE,
                           '\r',
                           base::string16(1, 0x3042));
  EXPECT_EQ(ASCIIToUTF16(""), character_composer_.preedit_string());

  // Sequence with an ignored character ('x') and Escape.
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 'U');
  EXPECT_EQ(ASCIIToUTF16("u"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, 0, '3');
  EXPECT_EQ(ASCIIToUTF16("u3"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, 0, '0');
  EXPECT_EQ(ASCIIToUTF16("u30"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_X, DomCode::KEY_X, 0, 'x');
  EXPECT_EQ(ASCIIToUTF16("u30"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, 0, '4');
  EXPECT_EQ(ASCIIToUTF16("u304"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, 0, '2');
  EXPECT_EQ(ASCIIToUTF16("u3042"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(VKEY_ESCAPE, DomCode::ESCAPE, EF_NONE, 0x1B);
  EXPECT_EQ(ASCIIToUTF16(""), character_composer_.preedit_string());
}

TEST_F(CharacterComposerTest, HexadecimalCompositionWithNonHexKey) {
  // Sequence [Ctrl+Shift+U, x, space] does not compose a character.
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_X, DomCode::KEY_X, 0, 'x');
  ExpectUnicodeKeyFiltered(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ');
  EXPECT_TRUE(character_composer_.composed_character().empty());

  // HIRAGANA LETTER A (U+3042) with a sequence [3, 0, x, 4, 2].
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, EF_NONE, '0');
  ExpectUnicodeKeyFiltered(VKEY_X, DomCode::KEY_X, EF_NONE, 'x');
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, EF_NONE, '4');
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ',
                           base::string16(1, 0x3042));
}

TEST_F(CharacterComposerTest, HexadecimalCompositionWithAdditionalModifiers) {
  // Ctrl+Shift+Alt+U
  // HIRAGANA LETTER A (U+3042)
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN | EF_ALT_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, EF_NONE, '0');
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, EF_NONE, '4');
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ',
                           base::string16(1, 0x3042));

  // Ctrl+Shift+u (CapsLock enabled)
  ExpectUnicodeKeyNotFiltered(
      VKEY_U, DomCode::KEY_U,
      EF_SHIFT_DOWN | EF_CONTROL_DOWN | EF_CAPS_LOCK_DOWN, 'u');
}

TEST_F(CharacterComposerTest, CancelHexadecimalComposition) {
  // Cancel composition with ESC.
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');
  ExpectUnicodeKeyFiltered(VKEY_ESCAPE, DomCode::ESCAPE, EF_NONE, 0x1B);

  // Now we can start composition again since the last composition was
  // cancelled.
  // HIRAGANA LETTER A (U+3042)
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, EF_NONE, '0');
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, EF_NONE, '4');
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ',
                           base::string16(1, 0x3042));
}

TEST_F(CharacterComposerTest, HexadecimalCompositionWithBackspace) {
  // HIRAGANA LETTER A (U+3042)
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, 0, '3');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, 0, '0');
  ExpectUnicodeKeyFiltered(VKEY_F, DomCode::KEY_F, 0, 'f');
  ExpectUnicodeKeyFiltered(VKEY_BACK, DomCode::BACKSPACE, EF_NONE, '\b');
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, EF_NONE, '4');
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ',
                           base::string16(1, 0x3042));
}

TEST_F(CharacterComposerTest, CancelHexadecimalCompositionWithBackspace) {
  // Backspace just after Ctrl+Shift+U.
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_BACK, DomCode::BACKSPACE, EF_NONE, '\b');
  ExpectUnicodeKeyNotFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');

  // Backspace twice after Ctrl+Shift+U and 3.
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, 0, '3');
  ExpectUnicodeKeyFiltered(VKEY_BACK, DomCode::BACKSPACE, EF_NONE, '\b');
  ExpectUnicodeKeyFiltered(VKEY_BACK, DomCode::BACKSPACE, EF_NONE, '\b');
  ExpectUnicodeKeyNotFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
}

TEST_F(CharacterComposerTest,
       HexadecimalCompositionPreeditWithModifierPressed) {
  // This test case supposes X Window System uses 101 keyboard layout.
  const int kControlShift = EF_CONTROL_DOWN | EF_SHIFT_DOWN;
  // HIRAGANA LETTER A (U+3042)
  ExpectUnicodeKeyFiltered(ui::VKEY_U, DomCode::KEY_U, kControlShift, 0x15);
  EXPECT_EQ(ASCIIToUTF16("u"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_3, DomCode::DIGIT3, kControlShift, '#');
  EXPECT_EQ(ASCIIToUTF16("u3"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_0, DomCode::DIGIT0, kControlShift, ')');
  EXPECT_EQ(ASCIIToUTF16("u30"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_4, DomCode::DIGIT4, kControlShift, '$');
  EXPECT_EQ(ASCIIToUTF16("u304"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_A, DomCode::KEY_A, kControlShift, 0x01);
  EXPECT_EQ(ASCIIToUTF16("u304a"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_BACK, DomCode::BACKSPACE, kControlShift,
                           '\b');
  EXPECT_EQ(ASCIIToUTF16("u304"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_2, DomCode::DIGIT2, kControlShift, 0);
  EXPECT_EQ(ASCIIToUTF16("u3042"), character_composer_.preedit_string());
  ExpectUnicodeKeyComposed(VKEY_RETURN, DomCode::ENTER, kControlShift,
                           '\r',
                           base::string16(1, 0x3042));
  EXPECT_EQ(ASCIIToUTF16(""), character_composer_.preedit_string());

  // Sequence with an ignored character (control + shift + 'x') and Escape.
  ExpectUnicodeKeyFiltered(ui::VKEY_U, DomCode::KEY_U, kControlShift, 'U');
  EXPECT_EQ(ASCIIToUTF16("u"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_3, DomCode::DIGIT3, kControlShift, '#');
  EXPECT_EQ(ASCIIToUTF16("u3"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_0, DomCode::DIGIT0, kControlShift, ')');
  EXPECT_EQ(ASCIIToUTF16("u30"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_X, DomCode::KEY_X, kControlShift, 'X');
  EXPECT_EQ(ASCIIToUTF16("u30"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_4, DomCode::DIGIT4, kControlShift, '$');
  EXPECT_EQ(ASCIIToUTF16("u304"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_2, DomCode::DIGIT2, kControlShift, 0);
  EXPECT_EQ(ASCIIToUTF16("u3042"), character_composer_.preedit_string());
  ExpectUnicodeKeyFiltered(ui::VKEY_ESCAPE, DomCode::ESCAPE, kControlShift,
                           0x1B);
  EXPECT_EQ(ASCIIToUTF16(""), character_composer_.preedit_string());
}

TEST_F(CharacterComposerTest, InvalidHexadecimalSequence) {
  // U+FFFFFFFF
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  for (int i = 0; i < 8; ++i)
    ExpectUnicodeKeyFiltered(VKEY_F, DomCode::KEY_F, 0, 'f');
  ExpectUnicodeKeyFiltered(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ');

  // U+0000 (Actually, this is a valid unicode character, but we don't
  // compose a string with a character '\0')
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  for (int i = 0; i < 4; ++i)
    ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, 0, '0');
  ExpectUnicodeKeyFiltered(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ');

  // U+10FFFF
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, 0, '0');
  for (int i = 0; i < 4; ++i)
    ExpectUnicodeKeyFiltered(VKEY_F, DomCode::KEY_F, 0, 'f');
  ExpectUnicodeKeyFiltered(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ');

  // U+110000
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');
  ExpectUnicodeKeyFiltered(VKEY_1, DomCode::DIGIT1, 0, '1');
  for (int i = 0; i < 4; ++i)
    ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, 0, '0');
  ExpectUnicodeKeyFiltered(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ');
}

TEST_F(CharacterComposerTest, HexadecimalSequenceAndDeadKey) {
  // LATIN SMALL LETTER A WITH ACUTE
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyComposed(VKEY_A, DomCode::KEY_A, EF_NONE, 'a',
                           base::string16(1, 0x00E1));
  // HIRAGANA LETTER A (U+3042) with dead_acute ignored.
  ExpectUnicodeKeyFiltered(VKEY_U, DomCode::KEY_U,
                           EF_SHIFT_DOWN | EF_CONTROL_DOWN, 0x15);
  ExpectUnicodeKeyFiltered(VKEY_3, DomCode::DIGIT3, EF_NONE, '3');
  ExpectUnicodeKeyFiltered(VKEY_0, DomCode::DIGIT0, EF_NONE, '0');
  ExpectDeadKeyFiltered(kCombiningAcute);
  ExpectUnicodeKeyFiltered(VKEY_4, DomCode::DIGIT4, EF_NONE, '4');
  ExpectUnicodeKeyFiltered(VKEY_2, DomCode::DIGIT2, EF_NONE, '2');
  ExpectUnicodeKeyComposed(VKEY_SPACE, DomCode::SPACE, EF_NONE, ' ',
                           base::string16(1, 0x3042));
}

}  // namespace ui
