#include "app.hpp"
#include "version.hpp"
#include "support_content.hpp"
#include <imgui.h>
#include <algorithm>
namespace shaker {
namespace {
void mutedText(const char* text){ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));ImGui::TextWrapped("%s",text);ImGui::PopStyleColor();}
void continueRow(const char* label){
    const float width=ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x;
    const float right=ImGui::GetCursorScreenPos().x+ImGui::GetContentRegionAvail().x;
    if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=right)ImGui::SameLine();
}
}
void App::renderSupport(const SupportIdentity& identity){
    const auto& c=supportContent();
    auto text=[](std::string value,const std::string& appName){const auto pos=value.find("{appName}");if(pos!=std::string::npos)value.replace(pos,9,appName);return value;};
    const auto logo=identity.appLogo;
    if(showSupport_){ImGui::OpenPopup("Feedback / Donate");showSupport_=false;}
    const auto viewport=ImGui::GetMainViewport();
    ImGui::SetNextWindowSize({std::min(680.f,viewport->Size.x-32),std::min(740.f,viewport->Size.y-32)},ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Appearing,{.5f,.5f});
    if(ImGui::BeginPopupModal("Feedback / Donate",nullptr,ImGuiWindowFlags_NoSavedSettings)){
        if(logo){ImGui::SetCursorPosX((ImGui::GetWindowWidth()-80)*.5f);ImGui::Image(ImTextureID(reinterpret_cast<uintptr_t>(logo)),{80,80});}
        const auto titleText=identity.appName;const char* title=titleText.c_str();
        ImGui::SetCursorPosX(std::max(0.f,(ImGui::GetWindowWidth()-ImGui::CalcTextSize(title).x)*.5f));ImGui::TextUnformatted(title);
        ImGui::Text("Version %s",identity.appVersion.c_str());
        const auto update=updates_.status();mutedText(update.message.c_str());
        auto link=[](const char* label,const std::string& url){if(ImGui::Button(label,{0,44}))ShellExecuteW(nullptr,L"open",wide(url).c_str(),nullptr,nullptr,SW_SHOWNORMAL);};
        link(c["actions"]["discord"].get<std::string>().c_str(),c["links"]["discord"].get<std::string>());
        continueRow("Check for updates");
        ImGui::BeginDisabled(update.state==UpdateState::Checking||update.state==UpdateState::Downloading);
        if(ImGui::Button("Check for updates",{0,44}))identity.checkForUpdates();ImGui::EndDisabled();
        continueRow("Feedback");if(ImGui::Button("Feedback",{0,44})){showFeedback_=true;ImGui::CloseCurrentPopup();}
        if(update.release&&compareVersions(update.release->version,appVersion)>0){
            if(ImGui::Button("Update details",{0,44})){showUpdates_=true;ImGui::CloseCurrentPopup();}
        }
        ImGui::Spacing();ImGui::TextWrapped("%s",text(c["intro"].get<std::string>(),identity.appName).c_str());
        ImGui::Spacing();ImGui::TextWrapped("%s",c["projectsText"].get<std::string>().c_str());
        link("GitHub",c["links"]["github"].get<std::string>());continueRow("AdamCh.com");link("AdamCh.com",c["links"]["website"].get<std::string>());
        ImGui::Separator();ImGui::TextUnformatted(c["donationTitle"].get<std::string>().c_str());
        ImGui::TextWrapped("%s",c["donationText"].get<std::string>().c_str());
        if(ImGui::BeginTable("DonationButtons",3,ImGuiTableFlags_SizingStretchSame)){
            for(const auto& platform:c["platforms"]){
                ImGui::TableNextColumn();link(platform["label"].get<std::string>().c_str(),platform["url"].get<std::string>());
                for(const auto& note:platform["notes"])mutedText(note.get<std::string>().c_str());
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();if(ImGui::Button("Close",{100,44}))ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if(showFeedback_){ImGui::OpenPopup("Feedback / feature request");showFeedback_=false;}
    ImGui::SetNextWindowSize({std::min(600.f,viewport->Size.x-32),std::min(610.f,viewport->Size.y-32)},ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Appearing,{.5f,.5f});
    if(ImGui::BeginPopupModal("Feedback / feature request",nullptr,ImGuiWindowFlags_NoSavedSettings)){
        auto status=feedback_.status();
        if(feedbackPending_&&status.state!=FeedbackState::Sending){
            feedbackPending_=false;if(status.state==FeedbackState::Sent)feedbackMessage_.fill(0);
        }
        const bool busy=status.state==FeedbackState::Sending;
        ImGui::TextWrapped("%s",text(c["feedback"]["intro"].get<std::string>(),identity.appName).c_str());
        ImGui::BeginDisabled(busy);
        ImGui::TextUnformatted("Name");ImGui::SetNextItemWidth(-1);ImGui::InputText("##FeedbackName",feedbackName_.data(),feedbackName_.size());
        ImGui::TextUnformatted("Email");ImGui::SetNextItemWidth(-1);ImGui::InputText("##FeedbackEmail",feedbackEmail_.data(),feedbackEmail_.size());
        mutedText("Your name and email are remembered on this computer.");
        ImGui::TextUnformatted("Message");ImGui::InputTextMultiline("##FeedbackMessage",feedbackMessage_.data(),feedbackMessage_.size(),{-1,150});
        ImGui::EndDisabled();
        mutedText(c["feedback"]["privacy"].get<std::string>().c_str());
        if(!feedbackError_.empty())ImGui::TextWrapped("%s",feedbackError_.c_str());
        if(!status.message.empty())ImGui::TextWrapped("%s",status.message.c_str());
        if(ImGui::Button("Close",{100,44}))ImGui::CloseCurrentPopup();continueRow("Send feedback");
        ImGui::BeginDisabled(busy);
        if(ImGui::Button("Send feedback",{0,44})){
            feedbackError_.clear();
            try{
                auto body=feedbackPayload(feedbackName_.data(),feedbackEmail_.data(),feedbackMessage_.data(),identity.appVersion,identity.appId);
                writeTextAtomic(dataDirectory()/L"feedback-contact.json",Json{{"name",body["name"]},{"email",body["email"]}}.dump(2));
                feedback_.send(body);feedbackPending_=true;
            }catch(const std::exception& error){feedbackError_=error.what();}
        }
        ImGui::EndDisabled();ImGui::EndPopup();
    }
}


}
