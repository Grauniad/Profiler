// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "GCGV_App.h"

#include <X11/Xlib.h>

#include "include/base/cef_logging.h"
#include <iostream>

namespace {

int XErrorHandlerImpl(Display *display, XErrorEvent *event) {
  LOG(WARNING)
        << "X error received: "
        << "type " << event->type << ", "
        << "serial " << event->serial << ", "
        << "error_code " << static_cast<int>(event->error_code) << ", "
        << "request_code " << static_cast<int>(event->request_code) << ", "
        << "minor_code " << static_cast<int>(event->minor_code);
  return 0;
}

int XIOErrorHandlerImpl(Display *display) {
  return 0;
}

}  // namespace


// Entry point function for all processes.
int main(int argc, char* argv[]) {
  // Provide CEF with command-line arguments.
  CefMainArgs main_args(argc, argv);
  
  // SimpleApp implements application-level callbacks. It will create the first
  // browser instance in OnContextInitialized() after CEF has initialized.
  CefRefPtr<GCGV_App> app(new GCGV_App);

  /************************************************************
   *                  Sub-process Handling
   ************************************************************/

  // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
  // that share the same executable. This function checks the command-line and,
  // if this is a sub-process, executes the appropriate logic.
  int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
  if (exit_code >= 0) {
    // The sub-process has completed so return here.
    return exit_code;
  }

  /************************************************************
   *                  Main (Browser) Handling
   ************************************************************/

  CefSettings settings;

  if ( !settings.remote_debugging_port)
  {
      settings.remote_debugging_port = 9222;
  }
  std::cout << "Remote port: " << settings.remote_debugging_port << std::endl;


  // Install xlib error handlers so that the application won't be terminated
  // on non-fatal errors.
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

  // Initialize CEF for the browser process.
  CefInitialize(main_args, settings, app.get(), NULL);

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
  CefRunMessageLoop();

  CefShutdown();

  return 0;
}
