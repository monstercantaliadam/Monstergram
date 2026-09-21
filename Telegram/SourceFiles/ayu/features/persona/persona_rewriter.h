// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QtCore/QString>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace Ayu::Persona {

[[nodiscard]] QString DefaultPrompt();
[[nodiscard]] QString DefaultApiKey();

class Rewriter final : public QObject {
public:
	explicit Rewriter(QObject *parent = nullptr);
	~Rewriter() override;

	[[nodiscard]] static Rewriter *Instance();

	void rewrite(
		const QString &text,
		const QString &apiKey,
		const QString &customPrompt,
		Fn<void(QString)> onSuccess,
		Fn<void(QString)> onFail);

private:
	QNetworkAccessManager _manager;
};

} // namespace Ayu::Persona
