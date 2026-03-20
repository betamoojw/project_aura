// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebMqttHandlers.h"

#include <WiFi.h>

#include "config/AppConfig.h"
#include "modules/MqttRuntime.h"
#include "web/WebMqttPage.h"
#include "web/WebMqttSaveUtils.h"
#include "web/WebTemplates.h"
#include "web/WebUiBridge.h"
#include "web/WebUiBridgeAdapters.h"

namespace WebMqttHandlers {

void handleRoot(WebHandlerContext &context,
                const WebResponseUtils::StreamContext &stream_context) {
    if (!context.server || !context.mqtt_runtime) {
        return;
    }
    const WebUiBridge::Snapshot web_ui_snapshot =
        context.web_ui_bridge ? context.web_ui_bridge->snapshot() : WebUiBridge::Snapshot{};
    switch (WebMqttPage::rootAccess(context.wifi_is_connected && context.wifi_is_connected(),
                                    web_ui_snapshot.mqtt_screen_open)) {
        case WebMqttPage::RootAccess::NotFound:
            context.server->send(404, "text/plain", "Not found");
            return;
        case WebMqttPage::RootAccess::Locked: {
            String html = FPSTR(WebTemplates::kMqttLockedPage);
            WebResponseUtils::sendHtmlStreamResilient(*context.server, html, stream_context);
            return;
        }
        case WebMqttPage::RootAccess::Ready:
            break;
    }

    const bool wifi_connected = context.wifi_is_connected ? context.wifi_is_connected() : false;
    const String mqtt_user = context.mqtt_user ? *context.mqtt_user : String();
    const String mqtt_pass = context.mqtt_pass ? *context.mqtt_pass : String();

    WebMqttPage::PageData page_data;
    page_data.wifi_connected = wifi_connected;
    page_data.wifi_enabled = context.wifi_enabled ? *context.wifi_enabled : false;
    page_data.mqtt_enabled = context.mqtt_user_enabled ? *context.mqtt_user_enabled : false;
    page_data.mqtt_connected = context.mqtt_runtime->isConnected();
    page_data.mqtt_retry_stage = context.mqtt_runtime->retryStage();
    page_data.device_id = context.mqtt_device_id ? *context.mqtt_device_id : String();
    page_data.device_ip = wifi_connected ? WiFi.localIP().toString() : String("---");
    page_data.host = context.mqtt_host ? *context.mqtt_host : String();
    page_data.port = context.mqtt_port ? *context.mqtt_port : Config::MQTT_DEFAULT_PORT;
    page_data.user = mqtt_user;
    page_data.pass = mqtt_pass;
    page_data.device_name = context.mqtt_device_name ? *context.mqtt_device_name : String();
    page_data.base_topic = context.mqtt_base_topic ? *context.mqtt_base_topic : String();
    page_data.anonymous = context.mqtt_anonymous ? *context.mqtt_anonymous
                                                 : (mqtt_user.isEmpty() && mqtt_pass.isEmpty());
    page_data.discovery = context.mqtt_discovery ? *context.mqtt_discovery : false;

    const String html = WebMqttPage::renderHtml(FPSTR(WebTemplates::kMqttPageTemplate), page_data);
    WebResponseUtils::sendHtmlStreamResilient(*context.server, html, stream_context);
}

void handleSave(WebHandlerContext &context,
                bool ota_busy,
                uint32_t deferred_action_delay_ms,
                WebDeferredActionsState &deferred_actions,
                const WebResponseUtils::StreamContext &stream_context) {
    if (!context.server || !context.web_ui_bridge || !context.mqtt_runtime) {
        return;
    }

    WebRequest &server = *context.server;
    if (!context.wifi_is_connected || !context.wifi_is_connected()) {
        WebResponseUtils::sendNoStoreText(server, 404, "Not found");
        return;
    }
    const WebUiBridge::Snapshot web_ui_snapshot =
        context.web_ui_bridge->snapshot();
    if (!web_ui_snapshot.mqtt_screen_open) {
        WebResponseUtils::sendNoStoreText(server, 409, "Open MQTT screen to enable");
        return;
    }
    if (ota_busy) {
        WebResponseUtils::sendNoStoreText(server, 503, "OTA in progress");
        return;
    }

    WebMqttSaveUtils::SaveInput save_input{};
    save_input.host = server.arg("host");
    save_input.port = server.arg("port");
    save_input.user = server.arg("user");
    save_input.pass = server.arg("pass");
    save_input.device_name = server.arg("name");
    save_input.base_topic = server.arg("topic");
    save_input.anonymous = server.hasArg("anonymous");
    save_input.discovery = server.hasArg("discovery");

    WebMqttSaveUtils::CurrentCredentials current_credentials{};
    current_credentials.user = context.mqtt_user ? *context.mqtt_user : String();
    current_credentials.pass = context.mqtt_pass ? *context.mqtt_pass : String();

    const WebMqttSaveUtils::ParseResult parse_result =
        WebMqttSaveUtils::parseSaveInput(save_input, current_credentials);
    if (!parse_result.success) {
        WebResponseUtils::sendNoStoreText(
            server, parse_result.status_code, parse_result.error_message.c_str());
        return;
    }

    const WebUiBridge::ApplyResult apply_result =
        context.web_ui_bridge->applyMqttSave(
            WebUiBridgeAdapters::toUiMqttSaveUpdate(parse_result.update));
    if (!apply_result.success) {
        WebResponseUtils::sendNoStoreText(
            server,
            apply_result.status_code,
            apply_result.error_message.isEmpty()
                ? "Failed to persist MQTT settings"
                : apply_result.error_message.c_str());
        return;
    }

    String html = FPSTR(WebTemplates::kMqttSavePage);
    WebResponseUtils::sendHtmlStream(server, html, stream_context);
    deferred_actions.scheduleMqttSync(millis(), deferred_action_delay_ms);
}

}  // namespace WebMqttHandlers
