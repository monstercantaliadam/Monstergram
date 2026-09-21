// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/persona/persona_rewriter.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkRequest>

namespace Ayu::Persona {
namespace {

constexpr auto kDefaultApiKey = "AQ.Ab8RN6KET0KS8cCBg2LkZgiR9z_9SIX1XZtWiD4QiQFaaTh66Q";
constexpr auto kModelName = "gemini-3.8-flash";
constexpr auto kApiEndpoint = "https://generativelanguage.googleapis.com/v1beta/interactions?key=%1";

} // namespace

QString DefaultApiKey() {
	return QString::fromLatin1(kDefaultApiKey);
}

QString DefaultPrompt() {
	return u"Sen bir metin dönüştürücüsün. Sana verilen mesajı aşağıdaki persona kurallarına göre yeniden yaz. Sadece ve sadece dönüştürülmüş mesajı yaz, tırnak işareti, açıklama veya ek bir şey ekleme.\n\n"
		"Persona Kuralları:\n"
		"- 20–25 yaş bandında, üniversite okumuş/okuyan hissi.\n"
		"- Türkçesi iyi ama kusursuz cümle kurmaya çalışmıyor.\n"
		"- “açıkçası, bu arada, bence, ya, gerçekten, biraz, direkt, aşırı” gibi kelimeleri doğal kullanıyor.\n"
		"- Yerinde küçük harfle başlayabilir: “ya bence…”, “evet evet biliyorum”\n"
		"- Gereksiz emoji yok. Arada :), 😭, ahahah, jsjsjs olabilir ama dozunda.\n"
		"- Her mesaja paragraf paragraf açıklama yapmaz.\n"
		"- Sürekli karşı tarafı onaylamaz; gerektiğinde “yok buna katılmıyorum” diyebilir.\n"
		"- Hafif ironi ve ince mizah olabilir.\n"
		"- İngilizce kelime kullanımı doğal: “random”, “vibe”, “literally”, “cringe”, “fair”, “actually” — ama her cümlede değil.\n"
		"- “Kesinlikle!”, “Harika bir soru!”, “Sana yardımcı olayım” gibi yapay zekâ kokan ifadeler ASLA olmayacak.\n\n"
		"Dönüştürülecek Mesaj: "_q;
}

Rewriter::Rewriter(QObject *parent)
: QObject(parent) {
}

Rewriter::~Rewriter() = default;

Rewriter *Rewriter::Instance() {
	static auto instance = Rewriter();
	return &instance;
}

void Rewriter::rewrite(
		const QString &text,
		const QString &apiKey,
		const QString &customPrompt,
		Fn<void(QString)> onSuccess,
		Fn<void(QString)> onFail) {
	if (text.trimmed().isEmpty()) {
		if (onFail) {
			onFail(text);
		}
		return;
	}

	const auto key = apiKey.isEmpty() ? DefaultApiKey() : apiKey;
	const auto prompt = customPrompt.isEmpty() ? DefaultPrompt() : customPrompt;
	const auto fullInput = prompt + text;

	auto requestObj = QJsonObject();
	requestObj[u"model"_q] = QString::fromLatin1(kModelName);
	requestObj[u"input"_q] = fullInput;

	const auto requestData = QJsonDocument(requestObj).toJson(QJsonDocument::Compact);

	const auto urlString = QString::fromLatin1(kApiEndpoint).arg(key);
	auto request = QNetworkRequest(QUrl(urlString));
	request.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);
	request.setTransferTimeout(15000);

	auto reply = _manager.post(request, requestData);
	connect(reply, &QNetworkReply::finished, this, [=] {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (onFail) {
				onFail(text);
			}
			return;
		}

		const auto responseData = reply->readAll();
		auto parseError = QJsonParseError();
		const auto doc = QJsonDocument::fromJson(responseData, &parseError);
		if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
			if (onFail) {
				onFail(text);
			}
			return;
		}

		const auto root = doc.object();
		const auto steps = root.value(u"steps"_q).toArray();
		auto outputText = QString();

		for (const auto &stepVal : steps) {
			if (!stepVal.isObject()) {
				continue;
			}
			const auto stepObj = stepVal.toObject();
			if (stepObj.value(u"type"_q).toString() == u"model_output"_q) {
				const auto contentArr = stepObj.value(u"content"_q).toArray();
				for (const auto &contentVal : contentArr) {
					if (!contentVal.isObject()) {
						continue;
					}
					const auto textVal = contentVal.toObject().value(u"text"_q).toString();
					if (!textVal.isEmpty()) {
						outputText = textVal.trimmed();
						break;
					}
				}
			}
			if (!outputText.isEmpty()) {
				break;
			}
		}

		if (outputText.isEmpty()) {
			if (onFail) {
				onFail(text);
			}
			return;
		}

		if (onSuccess) {
			onSuccess(outputText);
		}
	});
}

} // namespace Ayu::Persona
