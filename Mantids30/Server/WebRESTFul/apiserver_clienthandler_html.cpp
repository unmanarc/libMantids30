#include "apiserver_clienthandler.h"
#include <Mantids30/Core/Helpers/encoders.h>
#include <Mantids30/Core/Memory/streamable_string.h>
#include <Mantids30/Protocol/HTTP/rsp_status.h>
#include <memory>
#include <string>

using namespace Mantids30::Protocol;
using namespace Mantids30::Core::Memory;
using namespace Mantids30::Server::WebRESTFul;
using namespace Mantids30;
using namespace std;

Mantids30::Protocol::HTTP::Status::Code APIServer_ClientHandler::redirectUsingJS(const std::string &url)
{
    if (url == "#retokenize")
    {
        return Mantids30::Protocol::HTTP::Status::Code::S_200_OK;
    }

    std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableString> htmlOutput = std::make_shared<Mantids30::Core::Memory::Streams::StreamableString>();
    htmlOutput->writeString("<script>window.location.href = atob('" + Mantids30::Core::Helpers::Encoders::encodeToBase64(url) + "');</script>");
    serverResponse.setContentDataStreamer(htmlOutput);
    serverResponse.setContentType("text/html", true);

    return Mantids30::Protocol::HTTP::Status::Code::S_200_OK;
}

Mantids30::Protocol::HTTP::Status::Code APIServer_ClientHandler::showBrowserMessage(const std::string &title, const std::string &message, Mantids30::Protocol::HTTP::Status::Code returnCode)
{
    std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableString> sHTMLPayloadOut = createHTMLAlertMessage(title, message);
    serverResponse.setContentDataStreamer(sHTMLPayloadOut);
    serverResponse.setContentType("text/html", true);
    return returnCode;
}

std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableString> APIServer_ClientHandler::createHTMLAlertMessage(const std::string &title, const std::string &message)
{
    std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableString> sPayloadOut = std::make_shared<Mantids30::Core::Memory::Streams::StreamableString>();
    sPayloadOut->writeString(R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>)");
    sPayloadOut->writeString(title);
    sPayloadOut->writeString(R"(            </title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    color: #333;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    height: 100vh;
                    margin: 0;
                    background-color: #f4f4f4;
                }
                .container {
                    text-align: center;
                    max-width: 600px;
                    padding: 20px;
                    background-color: #fff;
                    border: 1px solid #ccc;
                    border-radius: 8px;
                    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
                }
                h1 {
                    color: #d9534f;
                }
                p {
                    font-size: 1.1em;
                    margin-top: 10px;
                }
                a {
                    color: #0275d8;
                    text-decoration: none;
                }
                a:hover {
                    text-decoration: underline;
                }
            </style>
        </head>
        <body>
            <div class="container">
    )");
    sPayloadOut->writeString(message);
    sPayloadOut->writeString(R"(
            </div>
        </body>
        </html>
    )");
    return sPayloadOut;
}