/*
 * gAndroidDialog.h
 *
 *  Created on: July 26, 2023
 *      Author: Metehan Gezer
 */

#ifndef GANDROIDDIALOG_H
#define GANDROIDDIALOG_H

#include <string>
#include <jni.h>
#include <functional>

enum DialogButton {
	/** The identifier for the positive button. */
	DIALOGBUTTON_POSITIVE = -1,
	DIALOGBUTTON_OK = DIALOGBUTTON_POSITIVE,
	DIALOGBUTTON_YES = DIALOGBUTTON_POSITIVE,
	/** The identifier for the negative button. */
	DIALOGBUTTON_NEGATIVE = -2,
	DIALOGBUTTON_NO = DIALOGBUTTON_NEGATIVE,
	/** The identifier for the neutral button. */
	DIALOGBUTTON_CANCEL = -3
};

using DialogButtonCallback = std::function<void(int, DialogButton)>;
using DialogCancelCallback = std::function<void(int)>;

/**
 * Creates a dialog with the button texts given. Button will be hidden if text is empty.
 *
 * @param dialogId Id of this dialog, should be unique and it's used to determine which dialog's callback will be called.
 * @param message Message of the dialog
 * @param title Title of the dialog
 * @param cancelText Text of the cancel button
 * @param negativeText Text of the negative button
 * @param positiveText Text of the positive button
 * @param buttonCallback Function to call when user clicked a button
 * @param dismissCallback Function to call when used dismissed the dialog (ex. back button)
 */
void gShowDialog(int dialogId, const std::string& message, const std::string& title = "",
				 const std::string& cancelText = "",
				 const std::string& negativeText = "",
				 const std::string& positiveText = "",
				 DialogButtonCallback buttonCallback = nullptr,
				 DialogCancelCallback dismissCallback = nullptr);


/**
 * Creates a dialog with the default buttons of the given type.
 *
 * @param dialogId Id of this dialog, should be unique and it's used to determine which dialog's callback will be called.
 * @param message Message of the dialog
 * @param title Title of the dialog
 * @param type Type of the dialog, should be one of these: "ok", "okcancel", "yesno", "yesnocancel"
 * @param buttonCallback Function to call when user clicked a button
 * @param dismissCallback Function to call when used dismissed the dialog (ex. back button)
 */
void gShowDialog(int dialogId, const std::string& message, const std::string& title = "",
				 const std::string& type = "ok",
				 DialogButtonCallback buttonCallback = nullptr,
				 DialogCancelCallback dismissCallback = nullptr);


#endif //GANDROIDDIALOG_H
