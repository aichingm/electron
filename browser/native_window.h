// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_H_
#define ATOM_BROWSER_NATIVE_WINDOW_H_

#include "base/basictypes.h"
#include "base/cancelable_callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "browser/native_window_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/image/image.h"
#include "vendor/brightray/browser/default_web_contents_delegate.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace brightray {
class InspectableWebContents;
}

namespace content {
class BrowserContext;
class WebContents;
}

namespace gfx {
class Point;
class Rect;
class Size;
}

namespace IPC {
class Message;
}

namespace atom {

class AtomJavaScriptDialogManager;
class DevToolsDelegate;
struct DraggableRegion;

class NativeWindow : public brightray::DefaultWebContentsDelegate,
                     public content::WebContentsObserver,
                     public content::NotificationObserver {
 public:
  typedef base::Callback<void(const std::vector<unsigned char>& buffer)>
      CapturePageCallback;

  class DialogScope {
   public:
    explicit DialogScope(NativeWindow* window)
        : window_(window) {
      if (window_ != NULL)
        window_->set_has_dialog_attached(true);
    }

    ~DialogScope() {
      if (window_ != NULL)
        window_->set_has_dialog_attached(false);
    }

   private:
    NativeWindow* window_;

    DISALLOW_COPY_AND_ASSIGN(DialogScope);
  };

  virtual ~NativeWindow();

  // Create window with existing WebContents.
  static NativeWindow* Create(content::WebContents* web_contents,
                              base::DictionaryValue* options);

  // Create window with new WebContents.
  static NativeWindow* Create(base::DictionaryValue* options);

  // Find a window from its process id and routing id.
  static NativeWindow* FromRenderView(int process_id, int routing_id);

  void InitFromOptions(base::DictionaryValue* options);

  virtual void Close() = 0;
  virtual void CloseImmediately() = 0;
  virtual void Move(const gfx::Rect& pos) = 0;
  virtual void Focus(bool focus) = 0;
  virtual bool IsFocused() = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual bool IsVisible() = 0;
  virtual void Maximize() = 0;
  virtual void Unmaximize() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;
  virtual void SetFullscreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() = 0;
  virtual void SetSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetSize() = 0;
  virtual void SetMinimumSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetMinimumSize() = 0;
  virtual void SetMaximumSize(const gfx::Size& size) = 0;
  virtual gfx::Size GetMaximumSize() = 0;
  virtual void SetResizable(bool resizable) = 0;
  virtual bool IsResizable() = 0;
  virtual void SetAlwaysOnTop(bool top) = 0;
  virtual bool IsAlwaysOnTop() = 0;
  virtual void Center() = 0;
  virtual void SetPosition(const gfx::Point& position) = 0;
  virtual gfx::Point GetPosition() = 0;
  virtual void SetTitle(const std::string& title) = 0;
  virtual std::string GetTitle() = 0;
  virtual void FlashFrame(bool flash) = 0;
  virtual void SetKiosk(bool kiosk) = 0;
  virtual bool IsKiosk() = 0;
  virtual bool HasModalDialog();
  virtual gfx::NativeWindow GetNativeWindow() = 0;

  virtual bool IsClosed() const { return is_closed_; }
  virtual void OpenDevTools();
  virtual void CloseDevTools();
  virtual bool IsDevToolsOpened();
  virtual void InspectElement(int x, int y);

  // Creates a new window to debug the devtools.
  virtual scoped_ptr<NativeWindow> DebugDevTools();

  virtual void FocusOnWebView();
  virtual void BlurWebView();
  virtual bool IsWebViewFocused();

  virtual bool SetIcon(const std::string& path);

  // Returns the process handle of render process, useful for killing the
  // render process manually
  virtual base::ProcessHandle GetRenderProcessHandle();

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  virtual void CapturePage(const gfx::Rect& rect,
                           const CapturePageCallback& callback);

  // The same with closing a tab in a real browser.
  //
  // Should be called by platform code when user want to close the window.
  virtual void CloseWebContents();

  content::WebContents* GetWebContents() const;
  content::WebContents* GetDevToolsWebContents() const;

  void AddObserver(NativeWindowObserver* obs) {
    observers_.AddObserver(obs);
  }

  void RemoveObserver(NativeWindowObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  bool has_frame() const { return has_frame_; }
  std::string node_integration() const { return node_integration_; }

  void set_has_dialog_attached(bool has_dialog_attached) {
    has_dialog_attached_ = has_dialog_attached;
  }

 protected:
  explicit NativeWindow(content::WebContents* web_contents,
                        base::DictionaryValue* options);

  brightray::InspectableWebContents* inspectable_web_contents() const {
    return inspectable_web_contents_.get();
  }

  void NotifyWindowClosed();
  void NotifyWindowBlur();

  // Called when the window needs to update its draggable region.
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) = 0;

  // Implementations of content::WebContentsDelegate.
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE;
  virtual content::JavaScriptDialogManager*
      GetJavaScriptDialogManager() OVERRIDE;
  virtual void BeforeUnloadFired(content::WebContents* tab,
                                 bool proceed,
                                 bool* proceed_to_fire_unload) OVERRIDE;
  virtual void RequestToLockMouse(content::WebContents* web_contents,
                                  bool user_gesture,
                                  bool last_unlocked_by_target) OVERRIDE;
  virtual bool CanOverscrollContent() const OVERRIDE;
  virtual void ActivateContents(content::WebContents* contents) OVERRIDE;
  virtual void DeactivateContents(content::WebContents* contents) OVERRIDE;
  virtual void LoadingStateChanged(content::WebContents* source) OVERRIDE;
  virtual void MoveContents(content::WebContents* source,
                            const gfx::Rect& pos) OVERRIDE;
  virtual void CloseContents(content::WebContents* source) OVERRIDE;
  virtual bool IsPopupOrPanel(
      const content::WebContents* source) const OVERRIDE;
  virtual void RendererUnresponsive(content::WebContents* source) OVERRIDE;
  virtual void RendererResponsive(content::WebContents* source) OVERRIDE;

  // Implementations of content::WebContentsObserver.
  virtual void RenderViewDeleted(content::RenderViewHost*) OVERRIDE;
  virtual void RenderProcessGone(base::TerminationStatus status) OVERRIDE;
  virtual void BeforeUnloadFired(const base::TimeTicks& proceed_time) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Implementations of content::NotificationObserver.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Whether window has standard frame.
  bool has_frame_;

  // Window icon.
  gfx::Image icon_;

 private:
  // Called when CapturePage has done.
  void OnCapturePageDone(const CapturePageCallback& callback,
                         bool succeed,
                         const SkBitmap& bitmap);

  void OnRendererMessage(const string16& channel,
                         const base::ListValue& args);

  void OnRendererMessageSync(const string16& channel,
                             const base::ListValue& args,
                             IPC::Message* reply_msg);

  // Notification manager.
  content::NotificationRegistrar registrar_;

  // Observers of this window.
  ObserverList<NativeWindowObserver> observers_;

  // The windows has been closed.
  bool is_closed_;

  // The security token of iframe.
  std::string node_integration_;

  // There is a dialog that has been attached to window.
  bool has_dialog_attached_;

  // Closure that would be called when window is unresponsive when closing,
  // it should be cancelled when we can prove that the window is responsive.
  base::CancelableClosure window_unresposive_closure_;

  base::WeakPtrFactory<NativeWindow> weak_factory_;

  scoped_ptr<DevToolsDelegate> devtools_delegate_;
  scoped_ptr<AtomJavaScriptDialogManager> dialog_manager_;
  scoped_ptr<brightray::InspectableWebContents> inspectable_web_contents_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindow);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_H_
