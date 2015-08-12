
# statusnotifier: GObject Status Notifier Item

Starting with Plasma Next, KDE doesn't support the XEmbed systray in favor of
their own Status Notifier Specification.

This little library allows to easily create a GObject to manage a
StatusNotifierItem, handling all the DBus interface and letting you simply deal
with the object's properties and signals.

You can simply create a new StatusNotifier using one of the helper function,
e.g. `status_notifier_new_from_icon_name()`, or simply creating an object as
usual - you then just need to make sure to specify property `id` :

	sn = (StatusNotifier *) g_object_new (TYPE_STATUS_NOTIFIER,
	     "id",                       "app-id",
	     "status",                   STATUS_NOTIFIER_STATUS_NEEDS_ATTENTION,
	     "main-icon-name",           "app-icon",
	     "attention-icon-pixbuf",    pixbuf,
	     "tooltip-title",            "My tooltip",
	     "tooltip-body",             "This is an item about <b>app</b>",
	     NULL);

You can also set properties (other than `id`) after creation. Once ready, call
`status_notifier_register()` to register the item on the session bus and to the
StatusNotifierWatcher.

If an error occurs, signal `registration-failed` will be emitted. On success,
property `state` will be `STATUS_NOTIFIER_STATE_REGISTERED`.

Once registered, you can change properties as needed, and the proper DBus
signal will be emitted to let visualizations (hosts) know, and connect to the
signals (such as `context-menu`) which will be emitted when the corresponding
DBus method was called.

Simple as that.

## Free Software

statusnotifier - Copyright (C) 2014 Olivier Brunel <jjk@jjacky.com>

statusnotifier is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

statusnotifier is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
statusnotifier (COPYING). If not, see http://www.gnu.org/licenses/

## Want to know more?

Some useful links if you're looking for more info:

- [official site](http://jjacky.com/statusnotifier "statusnotifier @ jjacky.com")

- [source code & issue tracker](https://github.com/jjk-jacky/statusnotifier "statusnotifier @ GitHub.com")

- [PKGBUILD in AUR](https://aur.archlinux.org/packages/statusnotifier "AUR: statusnotifier")

Plus, statusnotifier comes with html documentation.
