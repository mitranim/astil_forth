# BOT-ASSISTED

import sublime
import sublime_plugin
import codecs
import os
import signal
import subprocess as sub
import threading

PLUGIN_NAME = "astil_forth"
PANEL_NAME = "output." + PLUGIN_NAME # Prefix is required by API.
READ_SIZE = 64 * 1024

class Eval(threading.Thread):
  def __init__(self, panel, src, cwd):
    super().__init__(daemon=True)
    self.panel = panel
    self.src = src
    self.cwd = cwd
    self.proc = None
    self.canceled = False
    self.announce = False

  def cancel(self, announce=False):
    self.canceled = True
    self.announce |= announce
    self.kill()

  def kill(self):
    proc = self.proc
    if proc:
      try:
        os.killpg(proc.pid, signal.SIGKILL)
      except ProcessLookupError:
        pass

  def append(self, text):
    if text:
      self.panel.run_command("append", {"characters": text})

  def run(self):
    try:
      if self.canceled: return

      self.proc = sub.Popen(
        args=["astil", "lang.af", "--eval=" + self.src],
        stdout=sub.PIPE,
        stderr=sub.STDOUT,
        cwd=self.cwd,
        start_new_session=True,
      )

      if self.canceled:
        self.kill()

      dec = codecs.getincrementaldecoder("utf-8")(errors="replace")
      for chunk in iter(lambda: self.proc.stdout.read1(READ_SIZE), b""):
        if self.canceled: break
        self.append(dec.decode(chunk))
      if not self.canceled or self.announce:
        self.append(dec.decode(b"", final=True))
      self.append("[ok]")
    except OSError as err:
      self.kill()
      if not self.canceled:
        self.append(f"{err}")
    finally:
      failed = False
      if self.proc:
        failed = self.proc.wait() != 0
        self.proc = None
      if self.announce:
        self.append("[canceled]")
      if not self.canceled:
        sublime.status_message("Eval failed" if failed else "Eval finished")

class astil_forth_eval_selection(sublime_plugin.WindowCommand):
  eval = None

  def run(self, cancel=False):
    if cancel:
      self.cancel(True)
      return

    view = self.window.active_view()
    if not view: return

    for reg in view.sel():
      src = view.substr(reg)
      if not src: continue

      panel = self.window.create_output_panel(PLUGIN_NAME)
      self.window.run_command("show_panel", {"panel": PANEL_NAME})
      self.queue_eval(panel, src, guess_cwd(view))

  def queue_eval(self, panel, src, cwd):
    self.cancel()
    sublime.status_message("")
    self.eval = Eval(panel, src, cwd)
    self.eval.start()

  def cancel(self, announce=False):
    if self.eval:
      self.eval.cancel(announce)
      self.eval = None

class astil_forth_event_listener(sublime_plugin.EventListener):
  def on_post_window_command(self, win, cmd, args):
    canceling = (
      cmd == "astil_forth_eval_selection" and
      args and args.get("cancel")
    )
    if not canceling and win.active_panel() != PANEL_NAME:
      win.run_command("astil_forth_eval_selection", {"cancel": True})

  def on_pre_close_window(self, win):
    win.run_command("astil_forth_eval_selection", {"cancel": True})

def guess_cwd(view):
  window = view.window()
  if view.file_name():
    return os.path.dirname(view.file_name())
  if len(window.folders()):
    return window.folders()[0]
