#include <Arduino.h>

String createElement(String tag, String children, String attributes = "")
{
    // wrap children in tag and return tag
    return "<" + tag + " " + attributes + ">" + children + "</" + tag + ">";
}

String createInputAndLabel(String name, String label, String type = "text", String value = "")
{
    String input = createElement("input", "", "id='" + name + "' name='" + name + "' type='" + type + "' value='" + value + "'");
    String labelE = createElement("label", label, "for='" + name + "'");
    return createElement("div", labelE + input);
}

String createCheckboxInputAndLabel(String name, String label, bool checked = false)
{
    String input = createElement("input", "", "id='" + name + "' name='" + name + "' type='checkbox'" + (checked ? " checked" : ""));
    String labelE = createElement("label", label, "for='" + name + "'");
    return createElement("div", labelE + input);
}
// {
//     String input = createElement("input", "", "id='" + name + "' name='" + name + "' type='" + type + "' value='" + value + "'");
//     String labelE = createElement("label", label, "for='" + name + "'");
//     return createElement("div", labelE + input);
// }

String createFormWithButton(String inputs, String action = "/")
{
    return createElement("form", inputs + createElement("button", "Submit", "type='submit'"), "method='post' action='" + action + "'");
}

String createDiv(String children)
{
    return createElement("div", children);
}

String createBreak()
{
    return "<br>";
}

String createDivider()
{
    return createElement("hr", "");
}

String createPage(String appName, String body)
{
    return createElement("html", createElement("head", createElement("title", appName)) + createElement("body", createElement("h1", appName) + body));
}
