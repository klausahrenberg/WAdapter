#ifndef W_PAGE_H
#define W_PAGE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WPage {
public:

  WPage* next;
  WPage(const char* id, const char* title) {
    this->next = nullptr;
    this->id = id;
    this->title = title;
    this->showInMainMenu = true;
    this->onPrintPage = nullptr;
    this->onSubmittedPage = nullptr;
    this->targetAfterSubmitting = nullptr;
	}

	~WPage() {
		delete this->id;
		delete this->title;
	}


  typedef std::function<void(AsyncWebServerRequest*, Print*)> TCommandPage;
  virtual void printPage(AsyncWebServerRequest* request, Print* page) {
      if (onPrintPage) onPrintPage(request, page);
  }

  virtual void submittedPage(AsyncWebServerRequest* request, Print* page) {
      if (onSubmittedPage) onSubmittedPage(request, page);
  }

  bool hasSubmittedPage() {
    if (onSubmittedPage) return true; else return false;
  }

    void setPrintPage(TCommandPage onPrintPage) {
        this->onPrintPage = onPrintPage;
    }

    void setSubmittedPage(TCommandPage onSubmittedPage) {
        this->onSubmittedPage = onSubmittedPage;
    }

	const char* getId() {
		return id;
	}

	const char* getTitle() {
		return title;
	}

  bool isShowInMainMenu() {
    return showInMainMenu;
  }

  void setShowInMainMenu(bool showInMainMenu) {
    this->showInMainMenu = showInMainMenu;
  }

  WPage* getTargetAfterSubmitting() {
    return this->targetAfterSubmitting;
  }

  void setTargetAfterSubmitting(WPage* targetAfterSubmitting) {
    this->targetAfterSubmitting = targetAfterSubmitting;
  }

protected:

private:
	const char* id;
	const char* title;
  WPage* targetAfterSubmitting;
  bool showInMainMenu;
  TCommandPage onPrintPage;
  TCommandPage onSubmittedPage;
};

#endif
