/*
 * EEZ PSU Firmware
 * Copyright (C) 2017-present, Envox d.o.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once

#include "gui_page.h"

#define LIST_ITEMS_PER_PAGE 4

namespace eez {
namespace psu {
namespace gui {

class ChSettingsTriggerPage : public Page {
public:
	ChSettingsTriggerPage();

	data::Value getData(const data::Cursor &cursor, uint8_t id);

    void editTriggerMode();

    void editVoltageTriggerValue();
    void editCurrentTriggerValue();

    void toggleOutputState();

    void editTriggerOnListStop();

    void editListCount();

private:
    static void onFinishTriggerModeSet();
    static void onTriggerModeSet(uint8_t value);

    static void onTriggerOnListStopSet(uint8_t value);

    static void onVoltageTriggerValueSet(float value);
    static void onCurrentTriggerValueSet(float value);

    static void onListCountSet(float value);
    static void onListCountSetToInfinity();
};

class ChSettingsListsPage : public SetPage {
public:
	ChSettingsListsPage();

    int getListLength(uint8_t id);
    float *getFloatList(uint8_t id);
    data::Value getMin(const data::Cursor &cursor, uint8_t id);
    data::Value getMax(const data::Cursor &cursor, uint8_t id);
    data::Value getDef(const data::Cursor &cursor, uint8_t id);
	data::Value getData(const data::Cursor &cursor, uint8_t id);
    bool setData(const data::Cursor &cursor, uint8_t id, data::Value value);

    void previousPage();
    void nextPage();

    void edit();

    bool isFocusWidget(const WidgetCursor &widgetCursor);

	int getDirty();
	void set();
	void discard();

    bool onEncoder(int counter);
    bool onEncoderClicked();

    void showInsertMenu();
    void showDeleteMenu();

    void insertRowAbove();
    void insertRowBelow();

    void deleteRow();
    void clearColumn();
    void deleteRows();
    void deleteAll();

private:
    int m_listVersion;

    float m_voltageList[MAX_LIST_LENGTH];
    uint16_t m_voltageListLength;

    float m_currentList[MAX_LIST_LENGTH];
    uint16_t m_currentListLength;

    float m_dwellList[MAX_LIST_LENGTH];
    uint16_t m_dwellListLength;

    int m_iCursor;

    int getRowIndex();
    int getColumnIndex();
    int getPageIndex();
    uint16_t getMaxListLength();
    uint16_t getNumPages();
    int getCursorIndexWithinPage();
    uint8_t getDataIdAtCursor();
    int getCursorIndex(const data::Cursor &cursor, uint8_t id);
    void moveCursorToFirstAvailableCell();

    bool isFocusedValueEmpty();
    float getFocusedValue();
    void setFocusedValue(float value);
    static void onValueSet(float value);
    void doValueSet(float value);

    void insertRow(int iRow, int iCopyRow);

    static void onClearColumn();
    void doClearColumn();

    static void onDeleteRows();
    void doDeleteRows();

    static void onDeleteAll();
    void doDeleteAll();
};

}
}
} // namespace eez::psu::gui
