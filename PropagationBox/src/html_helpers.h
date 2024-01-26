#pragma once

String createElement(String tag, String children, String attributes = "");
String createInputAndLabel(String name, String label, String type = "text", String value = "");
String createCheckboxInputAndLabel(String name, String label, bool checked = false);
String createFormWithButton(String inputs, String action = "/");
String createDiv(String children);
String createBreak();
String createDivider();
String createPage(String appName, String body);