/*
	FBInk: FrameBuffer eInker, a tool to print text & images on eInk devices (Kobo/Kindle)
	Copyright (C) 2018 NiLuJe <ninuje@gmail.com>

	----

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "button_scan_cmd.h"

// Help message
static void
    show_helpmsg(void)
{
	printf(
	    "\n"
	    "Button Scan (w/ FBInk %s)\n"
	    "\n"
	    "Usage: button_scan [-phvq]\n"
	    "\n"
	    "Spits out x, y coordinates for the USB Connect button from Kobo's 'USB Plugged In' popup, optionally pressing it in the process.\n"
	    "\n"
	    "OPTIONS:\n"
	    "\t-p, --press\tGenerate an input event to automatically press the button.\n"
	    "\t-w, --wait\tWhile in the \"Connected\" state, wait for the end of this USBMS session, and detect the end of the content import process.\n"
	    "\t-u, --unplug\tExplicitly fake an USB unplug event to exit the USBMS session as soon as possible.\n"
	    "\t\t\tDoing this with a *real* USBMS sessions is a potentially terrible idea! This is aimed at purely faked USBMS sessions.\n"
	    "\t-b, --both\tALL THE THINGS! Do everything in a single shot (scan for the button, press it, and wait for the end of the USBMS session).\n"
	    "\t-h, --help\tShow this help message.\n"
	    "\t-v, --verbose\tToggle printing diagnostic messages.\n"
	    "\t-q, --quiet\tToggle hiding hardware setup messages, as well as the coordinates themselves.\n"
	    "\n",
	    fbink_version());
	return;
}

// Application entry point
int
    main(int argc, char* argv[])
{
	int                        opt;
	int                        opt_index;
	static const struct option opts[] = { { "press", no_argument, NULL, 'p' },  { "wait", no_argument, NULL, 'w' },
					      { "unplug", no_argument, NULL, 'u' }, { "both", no_argument, NULL, 'b' },
					      { "help", no_argument, NULL, 'h' },   { "verbose", no_argument, NULL, 'v' },
					      { "quiet", no_argument, NULL, 'q' },  { NULL, 0, NULL, 0 } };

	FBInkConfig fbink_config = { 0U };
	// Default to verbose for now
	fbink_config.is_verbose = true;

	bool press_button      = false;
	bool detect_import     = false;
	bool do_wait_for_usbms = false;
	bool force_unplug      = false;
	int  errfnd            = 0;

	while ((opt = getopt_long(argc, argv, "pwubhvq", opts, &opt_index)) != -1) {
		switch (opt) {
			case 'p':
				press_button = true;
				break;
			case 'w':
				do_wait_for_usbms = true;
				break;
			case 'u':
				force_unplug = true;
				break;
			case 'b':
				press_button  = true;
				detect_import = true;
				break;
			case 'v':
				fbink_config.is_verbose = !fbink_config.is_verbose;
				break;
			case 'q':
				fbink_config.is_quiet = !fbink_config.is_quiet;
				break;
			case 'h':
				show_helpmsg();
				return EXIT_SUCCESS;
				break;
			default:
				fprintf(stderr, "?? Unknown option code 0%o ??\n", (unsigned int) opt);
				errfnd = 1;
				break;
		}
	}

	if (errfnd == 1 || argc > optind) {
		show_helpmsg();
		return ERRCODE(EXIT_FAILURE);
	}

	// Assume success, until shit happens ;)
	int rv = EXIT_SUCCESS;

	// Open framebuffer and keep it around, then setup globals.
	int fbfd = -1;
	if (ERRCODE(EXIT_FAILURE) == (fbfd = fbink_open())) {
		fprintf(stderr, "Failed to open the framebuffer, aborting . . .\n");
		return ERRCODE(EXIT_FAILURE);
	}
	if (fbink_init(fbfd, &fbink_config) == ERRCODE(EXIT_FAILURE)) {
		fprintf(stderr, "Failed to initialize FBInk, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// And actually do stuff :)
	if (do_wait_for_usbms) {
		rv = fbink_wait_for_usbms_processing(fbfd, force_unplug);
	} else {
		rv = fbink_button_scan(fbfd, press_button, false);
		// If the button press was successful, optionally wait for the end of the USBMS session
		if (press_button && rv == EXIT_SUCCESS && detect_import) {
			rv = fbink_wait_for_usbms_processing(fbfd, force_unplug);
		}
	}

	// Cleanup
cleanup:
	if (fbink_close(fbfd) == ERRCODE(EXIT_FAILURE)) {
		fprintf(stderr, "Failed to close the framebuffer, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
	}

	return rv;
}
