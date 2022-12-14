#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <iostream>
#include <queue>
#include <random>
#include <set>

#include "animation.h"
#include "input.h"
#include "mates.h"
#include "rand.h"

#include <fstream>

InputState input;
sf::Font font;

#define ANIMS_SUPPORTED 128

namespace Editor
{
	sf::Texture texture;

	sf::Sprite spr_selected;

	std::vector<Animation> all_animations(ANIMS_SUPPORTED);
	Animation* anim = &all_animations[0];
	std::vector<AnimationData> all_animation_datas(ANIMS_SUPPORTED);
	AnimationData* data = &all_animation_datas[0];
	std::vector<std::string> all_animation_names(ANIMS_SUPPORTED);

	int anims_total = 1;
	int anim_selected = 0;

	int frame_current; 
	sf::IntRect* rect = &data->rect[0];
	int* time = &data->timer[0];

	sf::IntRect clipboard_rect;
	int clipboard_time;

	char buff_animation_name[128];

	void RefreshFrameSelected()
	{
		rect = &data->rect[frame_current];
		time = &data->timer[frame_current];
		spr_selected.setTextureRect(*rect);
	}

	void RefreshAnimationNameInputText()
	{
		strcpy_s(Editor::buff_animation_name, Editor::all_animation_names[Editor::anim_selected].c_str());
	}

	void CreateNewAnimationConfig()
	{
		data->frames = 1;
		frame_current = 0;
		data->timer[frame_current] = 1;
		data->rect[frame_current] = sf::IntRect(1, 1, 1, 1);
		RefreshFrameSelected();

		Editor::anim->data = Editor::data;
	}

	void AddNewAnim()
	{
		int last_anim = Editor::anims_total;
		Editor::anims_total++;
		AnimationData* last_data = &all_animation_datas[last_anim];
		last_data->frames = 1;
		last_data->timer[0] = 1;
		last_data->rect[0] = sf::IntRect(1, 1, 1, 1);

		all_animations[last_anim].data = last_data;
	}

	void UpdatePreview(sf::Time t)
	{
		anim->Update(t.asMilliseconds());
	}

	void AddNewFrame()
	{
		anim->data->frames++;
	}

	void DeleteLastFrame()
	{
		anim->data->frames--;
	}

	void SelectFrame(int f)
	{
		frame_current = f;
		RefreshFrameSelected();
	}

	void ShiftCurrentFrameLeft()
	{
		sf::IntRect tmp_rect = data->rect[frame_current];
		int tmp_timer = data->timer[frame_current];

		data->rect[frame_current] = data->rect[frame_current - 1];
		data->timer[frame_current] = data->timer[frame_current - 1];

		frame_current--;
		
		data->rect[frame_current] = tmp_rect;
		data->timer[frame_current] = tmp_timer;
	}

	void ShiftCurrentFrameRight()
	{
		sf::IntRect tmp_rect = data->rect[frame_current];
		int tmp_timer = data->timer[frame_current];

		data->rect[frame_current] = data->rect[frame_current + 1];
		data->timer[frame_current] = data->timer[frame_current + 1];

		frame_current++;

		data->rect[frame_current] = tmp_rect;
		data->timer[frame_current] = tmp_timer;
	}
	
	void LoadAnimationConfigFromFile(std::ifstream& file)
	{
		file >> Editor::anims_total;

		for (int i = 0; i < Editor::anims_total; ++i)
		{
			std::string anim_name;
			file >> anim_name;

			Editor::all_animation_names[i] = anim_name;

			int frames;
			file >> frames;
			Editor::all_animation_datas[i].frames = frames;

			AnimationData* data = &Editor::all_animation_datas[i];

			for (int j = 0; j < frames; ++j)
			{
				file >> data->rect[j].left;
				file >> data->rect[j].top;
				file >> data->rect[j].width;
				file >> data->rect[j].height;
				file >> data->timer[j];
			}

			Editor::all_animations[i].data = data;
		}

		file.close();

		Editor::RefreshFrameSelected();
		Editor::RefreshAnimationNameInputText();
	}

	void Save()
	{
		std::ofstream file;
		file.open("project.anm", ios::out | ios::trunc);

		file << Editor::anims_total << " ";

		for (int i = 0; i < Editor::anims_total; ++i)
		{
			file << Editor::all_animation_names[i] << " ";

			int frames = Editor::all_animation_datas[i].frames;
			file << frames << " ";

			AnimationData* data = &Editor::all_animation_datas[i];

			for (int j = 0; j < frames; ++j)
			{
				file << data->rect[j].left << " ";
				file << data->rect[j].top << " ";
				file << data->rect[j].width << " ";
				file << data->rect[j].height << " ";
				file << data->timer[j] << " ";
			}
		}

		file.close();
	}

	void Export()
	{
		std::ofstream file;
		file.open("anim_data.h", ios::out | ios::trunc);

		file << "#pragma once\n";

		file << "#include <SFML/Graphics.hpp>\n";

		file << "\n";

		file << "struct AnimationData\n";
		file << "{\n";
		file << "\tint frames;\n";
		file << "\tsf::IntRect rect[16];\n";
		file << "\tint timer[16];\n";
		file << "};";


		file << "\n";
		file << "\n";

		file << "enum AnimationType\n";
		file << "{\n";

		for (int i = 0; i < Editor::anims_total; ++i)
		{
			file << "\t" << Editor::all_animation_names[i] << ",\n";
		}

		file << "};\n";


		file << "\n";

		file << "AnimationData anim_lib[] = \n";
		file << "{\n";

		for (int i = 0; i < Editor::anims_total; ++i)
		{
			file << "\t//" << Editor::all_animation_names[i] << "\n";

			AnimationData* data = &Editor::all_animation_datas[i];

			file << "\t{\n";
			{
				file << "\t\t" << data->frames << ",\n";

				{
					file << "\t\t{\n";
					for (int j = 0; j < data->frames; ++j)
					{
						file << "\t\t\t{" << data->rect[j].left << ",";
						file << data->rect[j].top << ",";
						file << data->rect[j].width << ",";
						file << data->rect[j].height << "},\n";
					}
					file << "\t\t},\n";
				}
				{
					file << "\t\t{\n";
					file << "\t\t\t";
					for (int j = 0; j < data->frames; ++j)
					{
						file << data->timer[j] << ",";
					}
					file << "\n\t\t},\n";
				}

			}
			file << "\t},\n";
		}
		file << "};\n";

		file.close();
	}
};

sf::RenderTexture rtex_preview;

const int SCR_WIDTH = 1200;
const int SCR_HEIGHT = 800;

void InitialSetup(sf::RenderWindow& window)
{
	Editor::texture.loadFromFile("data/spritesheet.png");

	Editor::spr_selected.setTexture(Editor::texture);
	Editor::spr_selected.setTextureRect(sf::IntRect(16, 16, 16, 16));
	
	window.setFramerateLimit(60);
	ImGui::SFML::Init(window);

	input.Remap();

	rtex_preview.create(200, 200);

	Editor::CreateNewAnimationConfig();
}

void Image( const sf::FloatRect& textureRect, const sf::Color& tintColor = sf::Color::White, const sf::Color& borderColor = sf::Color::Transparent )
{
	sf::Vector2f textureSize = static_cast<sf::Vector2f>(Editor::texture.getSize());
	ImVec2 uv0(textureRect.left / textureSize.x, (textureRect.top + textureRect.height) / textureSize.y);
	ImVec2 uv1((textureRect.left + textureRect.width) / textureSize.x, textureRect.top / textureSize.y);
	ImGui::Image(rtex_preview.getTexture().getNativeHandle(), rtex_preview.getTexture().getSize(), uv0, uv1, tintColor, borderColor);
}

namespace UI_EditorSection
{
	void EditorSection_Editor()
	{
		sf::Sprite& spr = Editor::spr_selected;

		std::string editor_title = "editor.";
		if (input.IsMousePressed(1))
		{
			editor_title += " (" + std::to_string(sf::Mouse::getPosition().x) + ", " +
				std::to_string(sf::Mouse::getPosition().y) + ")";
		}

		ImGui::Text(editor_title.c_str());
		ImGui::NewLine();

		ImGui::Columns(2, "columns", false);

		spr.setScale(4, 4);
		ImGui::Image(spr, sf::Color::White, sf::Color::Red);
		ImGui::NextColumn();
		ImGui::PushItemWidth(100);
		ImGui::InputInt("X", &Editor::rect->left, 1, 1, 0);

		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputInt("W", &Editor::rect->width, 1, 1, 0);

		ImGui::PushItemWidth(100);
		ImGui::InputInt("Y", &Editor::rect->top, 1, 1, 0);
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputInt("H", &Editor::rect->height, 1, 1, 0);

		ImGui::InputInt("duration (ms)", Editor::time, 1, 1, 0);
		ImGui::NextColumn();

		ImGui::Columns(1, "columns", false);

	}
	void EditorSection_FrameSelector()
	{
		sf::Sprite& spr = Editor::spr_selected;

		ImGui::Text("selector.");

		{
			int frames = Editor::data->frames;
			if (frames > 1)
			{
				for (int i = 0; i < frames; ++i)
				{
					if (ImGui::RadioButton(std::to_string(i + 1).c_str(), &Editor::frame_current, i))
					{
						Editor::RefreshFrameSelected();
					}
					ImGui::SameLine();
				}
			}

			if (ImGui::Button("ADD"))
			{
				Editor::AddNewFrame();
			}

			if (frames > 1)
			{
				ImGui::SameLine();

				if (ImGui::Button("DELETE"))
				{
					Editor::DeleteLastFrame();
				}


				ImGui::SameLine();
				if (ImGui::Button("<<"))
				{
					if (Editor::frame_current > 0)
					{
						Editor::ShiftCurrentFrameLeft();
					}
				}

				ImGui::SameLine();
				if (ImGui::Button(">>"))
				{
					if (Editor::frame_current < (Editor::anims_total - 1))
					{
						Editor::ShiftCurrentFrameRight();
					}
				}

			}
		}

		//Show all frames
		for (int i = 0; i < Editor::data->frames; ++i)
		{
			if (i > 0) ImGui::SameLine();
			spr.setTextureRect(Editor::data->rect[i]);
			spr.setScale(2, 2);
			ImGui::Image(spr);

		}
	}
	void EditorSection_AnimationSelector()
	{

		ImGui::Columns(1, "columns", false);
		static int anim_sel = 0;

		if (ImGui::InputText("name", &Editor::buff_animation_name[0], 128))
		{
			Editor::all_animation_names[Editor::anim_selected] = std::string(Editor::buff_animation_name);
		}

		int anims = Editor::anims_total;
		if (anims > 1)
		{
			for (int i = 0; i < anims; ++i)
			{
				if (ImGui::RadioButton(("a" + std::to_string(i + 1)).c_str(), &anim_sel, i))
				{
					Editor::anim = &Editor::all_animations[i];
					Editor::data = &Editor::all_animation_datas[i];
					Editor::frame_current = 0;
					Editor::RefreshFrameSelected();
					Editor::anim_selected = i;

					Editor::RefreshAnimationNameInputText();
				}
				if (i == 0 || i % 7 != 0)
				{
					ImGui::SameLine();
				}
			}
		}

		if (ImGui::Button("ADD ANIM"))
		{
			Editor::AddNewAnim();
		}

		ImGui::Columns(2, "columns", false);
	}
}

void EditorStuff()
{

#if _DEBUG
	static bool show_demo = false;
	if (input.IsJustPressed(GameKeys::F1))
	{
		show_demo = !show_demo;
	}

	if (show_demo)
	{
		ImGui::ShowDemoWindow();
	}
#endif

	ImGui::Begin("animus", NULL, /*ImGuiWindowFlags_::ImGuiWindowFlags_NoMove  |*/ ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration);
	
	ImGui::Text("file.");
	ImGui::NewLine();

	if (ImGui::Button("SAVE CONFIG"))
	{
		Editor::Save();
	}

	ImGui::SameLine();

	if (ImGui::Button("EXPORT HEADER"))
	{
		Editor::Export();
	}

	ImGui::Separator();

	UI_EditorSection::EditorSection_Editor();

	ImGui::Separator();

	UI_EditorSection::EditorSection_FrameSelector();

	ImGui::Separator();

	ImGui::Text("preview.");
	ImGui::NewLine();

	//Show preview image
	sf::Sprite spr_preview;
	spr_preview.setTexture(Editor::texture);
	spr_preview.setTextureRect(Editor::anim->CurrentFrame());
	spr_preview.setScale(8, 8);
	ImGui::Image(spr_preview, sf::Vector2f(64, 64));

	ImGui::Separator();

	ImGui::Text("animations.");
	ImGui::NewLine();

	UI_EditorSection::EditorSection_AnimationSelector();

	ImGui::End();
}

void UpdateInput(sf::RenderWindow& window, int dt)
{
	static bool moving_rect = false;
	static sf::Vector2i moving_rect_st;
	static sf::Vector2i moving_rect_mp;
	sf::Vector2i mp = sf::Mouse::getPosition(window);

	if (input.IsJustMousePressed(0))
	{
		const int bg_texture_scale = 2;
		if (Editor::rect->contains(mp / bg_texture_scale))
		{
			moving_rect = true;
			moving_rect_st.x = Editor::rect->left;
			moving_rect_st.y = Editor::rect->top;
			moving_rect_mp = mp;
		}
	}

	if (moving_rect)
	{
		Editor::rect->left = (moving_rect_st.x + (-moving_rect_mp.x + mp.x)/2); 
		Editor::rect->top = (moving_rect_st.y + (-moving_rect_mp.y + mp.y)/2);
	}

	if (input.IsJustMouseReleased(0))
	{
		moving_rect = false;
	}

	if (input.IsJustMousePressed(1))
	{
		Editor::rect->left = sf::Mouse::getPosition(window).x / 2;
		Editor::rect->top = sf::Mouse::getPosition(window).y / 2;
	}

	if (input.IsMousePressed(1))
	{
		Editor::rect->width = (sf::Mouse::getPosition(window).x / 2 - Editor::rect->left);
		Editor::rect->height = (sf::Mouse::getPosition(window).y / 2 - Editor::rect->top);
	}

	if (input.IsPressed(GameKeys::CTRL))
	{
		if (input.IsJustPressed(GameKeys::C))
		{
			Editor::clipboard_rect = *Editor::rect;
			Editor::clipboard_time = *Editor::time;
		}
		if (input.IsJustPressed(GameKeys::V))
		{
			*Editor::rect = Editor::clipboard_rect;
			*Editor::time = Editor::clipboard_time;
		}
	}

	if (input.IsPressed(GameKeys::SHIFT))
	{
		if (input.IsJustPressed(GameKeys::RIGHT))
		{
			Editor::rect->width++;
		}
		if (input.IsJustPressed(GameKeys::LEFT))
		{
			Editor::rect->width--;
		}
		if (input.IsJustPressed(GameKeys::UP))
		{
			Editor::rect->height--;
		}
		if (input.IsJustPressed(GameKeys::DOWN))
		{
			Editor::rect->height++;
		}
	}
	else
	{
		static int timer_r = 0;
		static int timer_l = 0;
		static int timer_d = 0;
		static int timer_u = 0;

		const int LAUNCH_TIME = 200;
		if (input.IsJustPressed(GameKeys::RIGHT))
		{
			Editor::rect->left++;
			timer_r = -LAUNCH_TIME;
		}
		if (input.IsJustPressed(GameKeys::LEFT))
		{
			Editor::rect->left--;
			timer_l = -LAUNCH_TIME;
		}
		if (input.IsJustPressed(GameKeys::UP))
		{
			Editor::rect->top--;
			timer_u = -LAUNCH_TIME;
		}
		if (input.IsJustPressed(GameKeys::DOWN))
		{
			Editor::rect->top++;
			timer_d = -LAUNCH_TIME;
		}

		const int SNAP_TIME = 50;

		if (input.IsPressed(GameKeys::RIGHT))
		{
			timer_r += dt;
			if (timer_r > SNAP_TIME)
			{
				timer_r -= SNAP_TIME;
				Editor::rect->left++;
			}
		}
		if (input.IsPressed(GameKeys::LEFT))
		{
			timer_l += dt;
			if (timer_l > SNAP_TIME)
			{
				timer_l -= SNAP_TIME;
				Editor::rect->left--;
			}
		}
		if (input.IsPressed(GameKeys::UP))
		{
			timer_u += dt;
			if (timer_u > SNAP_TIME)
			{
				timer_u -= SNAP_TIME;
				Editor::rect->top--;
			}
		}
		if (input.IsPressed(GameKeys::DOWN))
		{
			timer_d += dt;
			if (timer_d > SNAP_TIME)
			{
				timer_d -= SNAP_TIME;
				Editor::rect->top++;
			}
		}
	}
}


void ProcessWindowEvents(sf::RenderWindow& window)
{
	sf::Event event;
	while (window.pollEvent(event))
	{
		ImGui::SFML::ProcessEvent(event);
		if (event.type == sf::Event::Closed)
		{
			window.close();
		}
	}
}

int main()
{
	sf::RenderWindow window(sf::VideoMode(SCR_WIDTH, SCR_HEIGHT), "animus");
	InitialSetup(window);

	sf::Clock clock_running_total;
	sf::Clock clock_one_frame;

	std::ifstream file;
	file.open("project.anm", ios::in);
	if (file.good())
	{
		Editor::LoadAnimationConfigFromFile(file);
	}
	file.close();

	while (window.isOpen())
	{
		Editor::texture.loadFromFile("data/spritesheet.png");

		const int time_general = clock_running_total.getElapsedTime().asMilliseconds();
		const sf::Time time_current_frame = clock_one_frame.restart();
		const int deltatime_frame = time_current_frame.asMilliseconds();

		ProcessWindowEvents(window);

		input.MakeSnapshot();
		UpdateInput(window, deltatime_frame);
		ImGui::SFML::Update(window, time_current_frame);

		Editor::UpdatePreview(time_current_frame);

		window.clear();

		static sf::Sprite main_texture; 
		main_texture.setTexture(Editor::texture);
		main_texture.setScale(2, 2);
		window.draw(main_texture);

		sf::RectangleShape shape_frame_selected;
		shape_frame_selected.setFillColor(sf::Color::Transparent);
		shape_frame_selected.setOutlineColor(sf::Color::Red);
		shape_frame_selected.setOutlineThickness(1);
		
		shape_frame_selected.setPosition(static_cast<float>(2*Editor::rect->left), static_cast<float>(2*Editor::rect->top));
		shape_frame_selected.setSize(sf::Vector2f(static_cast<float>(2*Editor::rect->width), static_cast<float>(2*Editor::rect->height)));
		window.draw(shape_frame_selected);

		Editor::RefreshFrameSelected();
		rtex_preview.clear();
		rtex_preview.draw(Editor::spr_selected);
		rtex_preview.display();

		EditorStuff();

		ImGui::SFML::Render(window);
		window.display();
	}

	ImGui::SFML::Shutdown();
}
