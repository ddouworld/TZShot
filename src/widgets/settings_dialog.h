#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QPointer>
#include <QDialog>

class AIViewModel;
class VisionViewModel;
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
class QCheckBox;
class QSpinBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(AIViewModel *aiViewModel,
                   VisionViewModel *visionViewModel,
                   StorageViewModel *storageViewModel,
                   LanguageManager *languageManager,
                   GifRecordViewModel *gifRecordViewModel,
                   OcrViewModel *ocrViewModel,
                   GlobalShortcut *globalShortcut,
                   QWidget *parent = nullptr);

    void showAndActivate();

signals:
    void infoMessageRequested(const QString &title, const QString &message);

private:
    QWidget *buildGeneralPage();
    QWidget *buildHotkeyPage();
    QWidget *createSectionTitle(const QString &title);
    QWidget *createCard();
    QWidget *createHotkeyRow(const QString &label, int actionId, const QString &seq);
    void populateVisionModelOptions(int provider, const QString &currentModel = QString());
    int aiProviderToVisionProvider(int aiProvider) const;
    int visionProviderToAiProvider(int visionProvider) const;
    QString shortcutForAction(int actionId) const;
    void refreshHotkeyPage();

    QPointer<AIViewModel> m_aiViewModel;
    QPointer<VisionViewModel> m_visionViewModel;
    QPointer<StorageViewModel> m_storageViewModel;
    QPointer<LanguageManager> m_languageManager;
    QPointer<GifRecordViewModel> m_gifRecordViewModel;
    QPointer<OcrViewModel> m_ocrViewModel;
    QPointer<GlobalShortcut> m_globalShortcut;

    QListWidget *m_navList = nullptr;
    QStackedWidget *m_stack = nullptr;

    QComboBox *m_modelCombo = nullptr;
    QLineEdit *m_apiKeyEdit = nullptr;
    QComboBox *m_visionModelCombo = nullptr;
    QCheckBox *m_visionWebSearchCheck = nullptr;
    QCheckBox *m_visionProxyEnabledCheck = nullptr;
    QComboBox *m_visionProxyTypeCombo = nullptr;
    QLineEdit *m_visionProxyHostEdit = nullptr;
    QSpinBox *m_visionProxyPortSpin = nullptr;
    QLabel *m_savePathLabel = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_gifPresetCombo = nullptr;
    QTextEdit *m_ocrSelfCheckText = nullptr;
    QWidget *m_hotkeyPage = nullptr;
};

#endif // SETTINGS_DIALOG_H
