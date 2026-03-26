#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QPointer>
#include <QDialog>

class AIViewModel;
class StorageViewModel;
class LanguageManager;
class GifRecordViewModel;
class OcrViewModel;
class GlobalShortcut;
class QListWidget;
class QStackedWidget;
class QLabel;
class QLineEdit;
class QComboBox;
class QTextEdit;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(AIViewModel *aiViewModel,
                   StorageViewModel *storageViewModel,
                   LanguageManager *languageManager,
                   GifRecordViewModel *gifRecordViewModel,
                   OcrViewModel *ocrViewModel,
                   GlobalShortcut *globalShortcut,
                   QWidget *parent = nullptr);

    void showAndActivate();

private:
    QWidget *buildGeneralPage();
    QWidget *buildHotkeyPage();
    QWidget *createSectionTitle(const QString &title);
    QWidget *createCard();
    QWidget *createHotkeyRow(const QString &label, int actionId, const QString &seq);
    QString shortcutForAction(int actionId) const;
    void refreshHotkeyPage();

    QPointer<AIViewModel> m_aiViewModel;
    QPointer<StorageViewModel> m_storageViewModel;
    QPointer<LanguageManager> m_languageManager;
    QPointer<GifRecordViewModel> m_gifRecordViewModel;
    QPointer<OcrViewModel> m_ocrViewModel;
    QPointer<GlobalShortcut> m_globalShortcut;

    QListWidget *m_navList = nullptr;
    QStackedWidget *m_stack = nullptr;

    QComboBox *m_modelCombo = nullptr;
    QLineEdit *m_apiKeyEdit = nullptr;
    QLabel *m_savePathLabel = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_gifPresetCombo = nullptr;
    QTextEdit *m_ocrSelfCheckText = nullptr;
    QWidget *m_hotkeyPage = nullptr;
};

#endif // SETTINGS_DIALOG_H
