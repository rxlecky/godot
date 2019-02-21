/*************************************************************************/
/*  script_watches.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "script_watches.h"

#include "core/math/expression.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/tool_button.h"

class ScriptWatch : public HBoxContainer {

	GDCLASS(ScriptWatch, HBoxContainer);

private:
	Label *number_label;
	LineEdit *expression_line;
	RichTextLabel *result_label;
	ToolButton *remove_button;
	int number;

	void _update_expression() {
		const String &new_expression = expression_line->get_text();

		if (!new_expression.empty()) {
			Expression expr;
			Error err;
			String result_str;
			bool restore = true;

			result_label->set_text("");
			err = expr.parse(new_expression);
			if (err == Error::OK) {
				Variant result = expr.execute(Array{});
				if (expr.has_execute_failed()) {
					result_label->push_color(get_color("error_color", "Editor"));
					Ref<Texture> icon = get_icon("Error", "EditorIcons");
					result_label->add_image(icon);
					result_label->add_text(" ");
					result_str = expr.get_error_text();
				} else {
					restore = false;
					result_str = result.get_construct_string();
				}
			} else {
				result_label->push_color(get_color("error_color", "Editor"));
				Ref<Texture> icon = get_icon("Error", "EditorIcons");
				result_label->add_image(icon);
				result_label->add_text(" ");
				result_str = expr.get_error_text();
			}
			result_label->add_text(result_str);
			if (restore) {
				result_label->pop();
			}

		} else {
			result_label->set_text("");
		}
	}

	void _expression_changed(const String new_expression) {
		_update_expression();
		emit_signal("expression_changed", this);
	}

	void _remove_pressed() {
		emit_signal("watch_removed", this);
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("_expression_changed", "new_expression"), &ScriptWatch::_expression_changed);
		ClassDB::bind_method(D_METHOD("_remove_pressed"), &ScriptWatch::_remove_pressed);

		ADD_SIGNAL(MethodInfo("expression_changed", PropertyInfo(Variant::OBJECT, "watch")));
		ADD_SIGNAL(MethodInfo("watch_removed", PropertyInfo(Variant::OBJECT, "watch")));
	}

	void _notification(int what) {
		switch (what) {
			case NOTIFICATION_ENTER_TREE:
				remove_button->set_icon(get_icon("Remove", "EditorIcons"));
		}
	}

public:
	void set_number(int watch_number) {
		number = watch_number;
		number_label->set_text(("#" + itos(watch_number).lpad(2, "0")));
	}

	String get_expression() const {
		return expression_line->get_text();
	}

	int get_number() const {
		return number;
	}

	ScriptWatch(int watch_number) {
		number_label = memnew(Label);
		number_label->set_text(("#" + itos(watch_number).lpad(2, "0")));
		add_child(number_label);

		expression_line = memnew(LineEdit);
		expression_line->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		expression_line->set_placeholder("Enter expression...");
		expression_line->connect("text_changed", this, "_expression_changed");
		add_child(expression_line);

		result_label = memnew(RichTextLabel);
		result_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		result_label->set_scroll_active(false);
		add_child(result_label);

		remove_button = memnew(ToolButton);
		remove_button->set_icon(get_icon("Error", "EditorIcons"));
		remove_button->connect("pressed", this, "_remove_pressed");
		add_child(remove_button);

		this->number = watch_number;
	}
};

void ScriptWatches::_watch_changed(Object *watch) {

	const ScriptWatch *w = cast_to<ScriptWatch>(watch);
	const String &new_expression = w->get_expression();

	if (!new_expression.empty()) {
		if (watch == watches[watches.size() - 1]) {
			_add_watch();
		}
	}
}

void ScriptWatches::_watch_removed(Object *watch) {
	const int num_watches = watches.size();
	ScriptWatch *w = cast_to<ScriptWatch>(watch);

	if (watches[num_watches - 1] != w) {
		_remove_watch(w);
	}
}

void ScriptWatches::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_watch_changed", "watch"), &ScriptWatches::_watch_changed);
	ClassDB::bind_method(D_METHOD("_watch_removed", "watch"), &ScriptWatches::_watch_removed);
}

void ScriptWatches::_add_watch() {
	ScriptWatch *watch = memnew(ScriptWatch(watches.size() + 1));
	watch->connect("expression_changed", this, "_watch_changed");
	watch->connect("watch_removed", this, "_watch_removed");
	main_vbox->add_child(watch);
	watches.push_back(watch);
}

void ScriptWatches::_remove_watch(ScriptWatch *watch) {
	int number = watch->get_number();

	watches.erase(watch);
	memdelete(watch);
	_update_watch_numbers(number - 1);
}

void ScriptWatches::_update_watch_numbers(int from_index) {
	const int size = watches.size();

	for (int i = from_index; i < size; i++) {
		watches[i]->set_number(i + 1);
	}
}

ScriptWatches::ScriptWatches() {
	main_vbox = memnew(VBoxContainer);
	main_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(main_vbox);

	_add_watch();
}
