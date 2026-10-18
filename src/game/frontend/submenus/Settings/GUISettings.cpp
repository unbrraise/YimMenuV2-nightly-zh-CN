#include "GUISettings.hpp"
#include "core/commands/ColorCommand.hpp"
#include "core/commands/Command.hpp"
#include "core/commands/FloatCommand.hpp"
#include "core/frontend/manager/styles/Themes.hpp"
#include <regex>

namespace YimMenu
{
	static std::vector<std::unique_ptr<ColorCommand>> g_ColorCommands;
	static std::unordered_map<std::string, float> g_RoundingValues;
	static std::unordered_map<std::string, std::unique_ptr<FloatCommand>> g_FloatCommands; // Global map for all FloatCommands
	static bool g_ColorInit = false, g_RoundingInit = false;

	static const std::string kSettingsFile = [] {
		char* path = nullptr;
		size_t len = 0;
		_dupenv_s(&path, &len, "APPDATA");
		std::string full = std::string(path ? path : "") + "\\YimMenuV2\\themes.json";
		free(path);
		return full;
	}();

	static std::string PrettyPrintLabel(const std::string& raw)
	{
		std::string out = raw;
		if (out.size() > 2 && out.compare(out.size() - 2, 2, "_X") == 0)
			out.replace(out.size() - 2, 2, " 水平");
		else if (out.size() > 2 && out.compare(out.size() - 2, 2, "_Y") == 0)
			out.replace(out.size() - 2, 2, " 垂直");

		std::string spaced;
		spaced.reserve(out.size() + 10);
		for (size_t i = 0; i < out.size(); ++i)
		{
			if (i > 0 && isupper(out[i]) && islower(out[i - 1]))
				spaced += ' ';
			spaced += out[i];
		}

		std::regex uscore_re("_");
		spaced = std::regex_replace(spaced, uscore_re, " ");

		return spaced;
	}

	void SyncColorCommandsToStyle()
	{
		auto& style = ImGui::GetStyle();
		for (int i = 0; i < ImGuiCol_COUNT; i++)
			style.Colors[i] = g_ColorCommands[i]->GetState();
	}

	static void SyncRoundingToStyle()
	{
		auto& s = ImGui::GetStyle();
		for (auto& [k, v] : g_RoundingValues)
			if (k == "窗口圆角")
				s.WindowRounding = v;
			else if (k == "框架圆角")
				s.FrameRounding = v;
			else if (k == "抓取圆角")
				s.GrabRounding = v;
			else if (k == "滚动条圆角")
				s.ScrollbarRounding = v;
			else if (k == "子窗口圆角")
				s.ChildRounding = v;
			else if (k == "弹出窗口圆角")
				s.PopupRounding = v;
			else if (k == "标签圆角")
				s.TabRounding = v;
	}

	static void LoadSettings()
	{
		if (!std::filesystem::exists(kSettingsFile))
			return;
		std::ifstream file(kSettingsFile);
		nlohmann::json json;
		file >> json;

		// Load colors
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
			if (auto it = json.find(ImGui::GetStyleColorName(i)); it != json.end() && it->is_array())
				g_ColorCommands[i]->SetState(ImVec4((*it)[0], (*it)[1], (*it)[2], (*it)[3]));

		// Load rounding values
		for (const char* key : {"窗口圆角", "框架圆角", "抓取圆角", "滚动条圆角", "子窗口圆角", "弹出窗口圆角", "标签圆角"})
			if (auto it = json.find(key); it != json.end())
				g_RoundingValues[key] = *it;

		// Load all float command values (except colors and rounding, which are handled)
		for (auto& [key, cmd] : g_FloatCommands)
		{
			if (auto it = json.find(key); it != json.end() && it->is_number())
			{
				cmd->SetState(it->get<float>());
			}
		}
	}

	static void SaveSettings()
	{
		nlohmann::json json;

		// Save colors
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			auto c = g_ColorCommands[i]->GetState();
			json[ImGui::GetStyleColorName(i)] = {c.x, c.y, c.z, c.w};
		}

		// Save rounding
		for (auto& [k, v] : g_RoundingValues)
			json[k] = v;

		// Save floats
		for (auto& [k, cmd] : g_FloatCommands)
			json[k] = cmd->GetState();

		std::filesystem::create_directories(std::filesystem::path(kSettingsFile).parent_path());
		std::ofstream(kSettingsFile) << json.dump(4);
	}

	void ApplyThemeToImGui()
	{
		auto& style = ImGui::GetStyle();

		for (int i = 0; i < ImGuiCol_COUNT; ++i)
			style.Colors[i] = g_ColorCommands[i]->GetState();

		style.WindowRounding = g_RoundingValues["窗口圆角"];
		style.FrameRounding = g_RoundingValues["框架圆角"];
		style.GrabRounding = g_RoundingValues["抓取圆角"];
		style.ScrollbarRounding = g_RoundingValues["滚动条圆角"];
		style.ChildRounding = g_RoundingValues["子窗口圆角"];
		style.PopupRounding = g_RoundingValues["弹出窗口圆角"];
		style.TabRounding = g_RoundingValues["标签圆角"];
	}


	void InitializeColorCommands()
	{
		if (g_ColorInit)
			return;
		auto& style = ImGui::GetStyle();
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
			g_ColorCommands.emplace_back(std::make_unique<ColorCommand>(
			    "ColorCommand." + std::string(ImGui::GetStyleColorName(i)),
			    ImGui::GetStyleColorName(i),
			    "编辑颜色：" + std::string(ImGui::GetStyleColorName(i)),
			    style.Colors[i]));

		if (!g_RoundingInit)
		{
			g_RoundingInit = true;
			g_RoundingValues = {
			    {"窗口圆角", style.WindowRounding},
			    {"框架圆角", style.FrameRounding},
			    {"抓取圆角", style.GrabRounding},
			    {"滚动条圆角", style.ScrollbarRounding},
			    {"子窗口圆角", style.ChildRounding},
			    {"弹出窗口圆角", style.PopupRounding},
			    {"标签圆角", style.TabRounding}};
		}

		LoadSettings();
		SyncColorCommandsToStyle();
		SyncRoundingToStyle();
		g_ColorInit = true;
	}

	static void DrawStyleVec2(const char* label, float& x, float& y, float min, float max)
	{
		std::string nameX = std::string(label) + "_X";
		std::string nameY = std::string(label) + "_Y";

		// Use global float commands map
		if (!g_FloatCommands.count(nameX))
			g_FloatCommands[nameX] = std::make_unique<FloatCommand>(nameX.c_str(), nameX.c_str(), "调整 " + nameX, min, max, x);
		if (!g_FloatCommands.count(nameY))
			g_FloatCommands[nameY] = std::make_unique<FloatCommand>(nameY.c_str(), nameY.c_str(), "调整 " + nameY, min, max, y);

		float newX = g_FloatCommands[nameX]->GetState();
		float newY = g_FloatCommands[nameY]->GetState();

		bool changed = false;
		changed |= ImGui::SliderFloat(PrettyPrintLabel(nameX).c_str(), &newX, min, max, "%.1f");
		changed |= ImGui::SliderFloat(PrettyPrintLabel(nameY).c_str(), &newY, min, max, "%.1f");

		if (changed)
		{
			g_FloatCommands[nameX]->SetState(newX);
			g_FloatCommands[nameY]->SetState(newY);
			x = newX;
			y = newY;
			SaveSettings();
		}
	}

	static void DrawStyleFloat(const char* label, float& v, float min, float max)
	{
		std::string name(label);

		if (!g_FloatCommands.count(name))
			g_FloatCommands[name] = std::make_unique<FloatCommand>(name.c_str(), name.c_str(), "调整 " + name, min, max, v);

		float newVal = g_FloatCommands[name]->GetState();
		if (ImGui::SliderFloat(PrettyPrintLabel(name).c_str(), &newVal, min, max, "%.1f"))
		{
			g_FloatCommands[name]->SetState(newVal);
			v = newVal;
			SaveSettings();
		}
	}

	static void DrawColorsTab()
	{
		bool changed = false;
		ImGui::Text("调整颜色：");
		ImGui::Separator();
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			auto& cmd = g_ColorCommands[i];
			auto col = cmd->GetState();

			if (ImGui::ColorEdit4(PrettyPrintLabel(ImGui::GetStyleColorName(i)).c_str(), (float*)&col))
			{
				cmd->SetState(col);
				changed = true;
			}
		}
		if (changed)
		{
			SyncColorCommandsToStyle();
			SaveSettings();
		}
	}

	static void DrawRoundingTab()
	{
		bool changed = false;
		ImGui::Text("调整圆角：");
		ImGui::Separator();
		for (auto& [k, v] : g_RoundingValues)
			if (ImGui::SliderFloat(PrettyPrintLabel(k).c_str(), &v, 0.0f, 20.0f, "%.1f"))
				changed = true;

		if (changed)
		{
			SyncRoundingToStyle();
			SaveSettings();
		}
	}

	static void DrawLayoutTab()
	{
		auto& s = ImGui::GetStyle();
		ImGui::Text("布局与对齐：");
		ImGui::Separator();
		DrawStyleVec2("窗口内边距", s.WindowPadding.x, s.WindowPadding.y, 0.f, 32.f);
		DrawStyleVec2("项目间距", s.ItemSpacing.x, s.ItemSpacing.y, 0.f, 32.f);
		DrawStyleVec2("项目内部间距", s.ItemInnerSpacing.x, s.ItemInnerSpacing.y, 0.f, 32.f);
		DrawStyleVec2("触控额外边距", s.TouchExtraPadding.x, s.TouchExtraPadding.y, 0.f, 32.f);
		DrawStyleVec2("显示安全区边距", s.DisplaySafeAreaPadding.x, s.DisplaySafeAreaPadding.y, 0.f, 32.f);

		DrawStyleFloat("缩进间距", s.IndentSpacing, 0.f, 64.f);
		DrawStyleFloat("列最小间距", s.ColumnsMinSpacing, 0.f, 64.f);

		DrawStyleVec2("窗口标题对齐", s.WindowTitleAlign.x, s.WindowTitleAlign.y, 0.f, 1.f);
		DrawStyleVec2("按钮文本对齐", s.ButtonTextAlign.x, s.ButtonTextAlign.y, 0.f, 1.f);
		DrawStyleVec2("可选项文本对齐", s.SelectableTextAlign.x, s.SelectableTextAlign.y, 0.f, 1.f);
	}

	static void DrawBorderTab()
	{
		auto& s = ImGui::GetStyle();
		ImGui::Text("边框尺寸：");
		ImGui::Separator();
		DrawStyleFloat("窗口边框大小", s.WindowBorderSize, 0.f, 8.f);
		DrawStyleFloat("框架边框大小", s.FrameBorderSize, 0.f, 8.f);
		DrawStyleFloat("标签边框大小", s.TabBorderSize, 0.f, 8.f);
		DrawStyleFloat("弹出窗口边框大小", s.PopupBorderSize, 0.f, 8.f);
	}

	static void DrawGlobalTab()
	{
		auto& s = ImGui::GetStyle();
		ImGui::Text("全局设置：");
		ImGui::Separator();
		DrawStyleFloat("全局透明度", s.Alpha, 0.1f, 1.f);
		DrawStyleFloat("禁用透明度", s.DisabledAlpha, 0.f, 1.f);
		DrawStyleFloat("鼠标光标缩放", s.MouseCursorScale, 0.5f, 2.f);
		DrawStyleFloat("曲线细分容差", s.CurveTessellationTol, 0.1f, 10.f);
	}

	static void DrawFontTab()
	{
		ImGuiIO& io = ImGui::GetIO();
		static float scale = io.FontGlobalScale;
		ImGui::Text("字体配置：");
		ImGui::Separator();
		ImGui::Text("当前缩放：%.2f", io.FontGlobalScale);
		ImGui::SliderFloat("字体缩放", &scale, 0.5f, 2.0f, "%.2f");
		if (ImGui::Button("应用字体缩放"))
			io.FontGlobalScale = scale;
	}

	std::shared_ptr<Category> DrawGUISettingsMenu()
	{
		InitializeColorCommands();
		auto imGuiCustomStyle = std::make_shared<Category>("自定义");
		imGuiCustomStyle->AddItem(std::make_unique<ImGuiItem>([] {
			ImGui::Text("界面样式编辑器");
			ImGui::Separator();
			if (ImGui::BeginTabBar("StyleTabs"))
			{
				if (ImGui::BeginTabItem("颜色"))
				{
					DrawColorsTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("圆角"))
				{
					DrawRoundingTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("布局"))
				{
					DrawLayoutTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("边框"))
				{
					DrawBorderTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("全局"))
				{
					DrawGlobalTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("字体"))
				{
					DrawFontTab();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}));
		return imGuiCustomStyle;
	}
}
