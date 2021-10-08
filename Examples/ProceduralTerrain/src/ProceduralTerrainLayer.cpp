
#include "ProceduralTerrainLayer.h"

#include <ctime>

using namespace GLCore;
using namespace GLCore::Utils;

GLuint vertexArrayHandle;
GLuint vertexBufferHandle;

unsigned int JenkinsOneAtATimeHash(const unsigned char *key, size_t length) 
{
	size_t i = 0;
	unsigned int hash = 0;
	while (i != length) {
		hash += key[i++];
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}

ProceduralTerrainLayer::ProceduralTerrainLayer()
{
}

ProceduralTerrainLayer::~ProceduralTerrainLayer()
{
}

void ProceduralTerrainLayer::OnAttach()
{
	EnableGLDebugging();

	std::time_t result = std::time(nullptr);
	m_Seed = (unsigned int) result;
	m_DockSpace = CreateRef<ImGuiExt::ViewportDockSpace>();

	m_Shader = Shader::FromGLSLTextFiles(
		"res/terrain-vert.glsl",
		"res/terrain-frag.glsl"
	);
	
	GLuint program = m_Shader->GetRendererID();
	glUseProgram(program);

	GLint loc = glGetUniformLocation(program, "u_Resolution");
	glUniform2f(loc, (float) Application::Get().GetWindow().GetWidth(), (float) Application::Get().GetWindow().GetHeight());

	loc = glGetUniformLocation(program, "u_Zoom");
	glUniform1f(loc, m_Zoom);

	loc = glGetUniformLocation(program, "u_Evolve");
	glUniform1f(loc, m_Evolve);

	loc = glGetUniformLocation(program, "u_Seed");
	glUniform1ui(loc, m_Seed);

	loc = glGetUniformLocation(program, "u_Scale");
	glUniform1f(loc, m_Scale);

	loc = glGetUniformLocation(program, "u_Lacunarity");
	glUniform1f(loc, m_Lacunarity);

	loc = glGetUniformLocation(program, "u_Persistence");
	glUniform1f(loc, m_Persistence);

	loc = glGetUniformLocation(program, "u_Octaves");
	glUniform1ui(loc, (GLuint) m_Octaves);

	float vertices[] =
	{
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f
	};

	glCreateVertexArrays(1, &vertexArrayHandle);
	glBindVertexArray(vertexArrayHandle);

	glCreateBuffers(1, &vertexBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

void ProceduralTerrainLayer::OnDetach()
{
	glDeleteVertexArrays(1, &vertexArrayHandle);
	glDeleteBuffers(1, &vertexBufferHandle);
}

void ProceduralTerrainLayer::OnEvent(Event &event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent e) 
	{
		glViewport(0, 0, e.GetWidth(), e.GetHeight());
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Resolution");
		glUniform2f(loc, (float) e.GetWidth(), (float) e.GetHeight());
		m_DockSpace->SetViewport({ e.GetWidth(), e.GetHeight() });
		return true;
	});

	dispatcher.Dispatch<KeyReleasedEvent>([&](KeyReleasedEvent e) 
	{
		if (e.GetKeyCode() == 256)
		{
			m_Fullscreen = false;
			return true;
		}

		return false;
	});
}

void ProceduralTerrainLayer::OnUpdate(Timestep ts)
{
	glClear(GL_COLOR_BUFFER_BIT);

	if (!m_Fullscreen)
	{
		DOCKSPACE_CAPTURE_SCOPE(m_DockSpace);
		glBindVertexArray(vertexArrayHandle);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		return;
	}

	glBindVertexArray(vertexArrayHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ProceduralTerrainLayer::OnImGuiRender()
{

	if (m_Fullscreen)
		return;

	m_DockSpace->Begin();
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Fullscreen", nullptr, m_Fullscreen))
				m_Fullscreen = true;

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	m_DockSpace->UpdateViewportWindow();

	ImGui::Begin("Controls");

	static char buffer[128] = { 0 };
	if (ImGui::InputText("Seed Hasher", buffer, sizeof(buffer)))
	{
		m_Seed = JenkinsOneAtATimeHash((unsigned char *)buffer, strlen(buffer));
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Seed");
		glUniform1ui(loc, m_Seed);
	}
	ImGuiExt::HelpMarker("Seed Hasher: Enter any text you like and it will be hashed into a 32-bit seed.");

	if (ImGui::InputInt("Seed", &m_Seed, 1, 100))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Seed");
		glUniform1ui(loc, m_Seed);
	}
	ImGuiExt::HelpMarker("Seed: Determines the base seed for the RNG (Random Number Generator). Changing the seed changes the output of the RNG.");

	ImGui::SameLine();
	if (ImGuiExt::Knob("Evolve", &m_Evolve, 0.0f, 100.0f, 0.02f))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Evolve");
		glUniform1f(loc, m_Evolve);
	}
	ImGuiExt::HelpMarker("Evolution: Evolves the output of the noise generator.");

	ImGui::Separator();
	if (ImGuiExt::Knob("Zoom", &m_Zoom, 0.05f, 50.0f))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Zoom");
		glUniform1f(loc, m_Zoom);
	}

	ImGui::Separator();
	if (ImGuiExt::Knob("Scale", &m_Scale, 1.0f, 100.0f))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Scale");
		glUniform1f(loc, m_Scale);
	}
	ImGuiExt::HelpMarker("Scale: Changes the output scale/zoom of the noise generator. The larger the scale, the more output you'll see.");

	ImGui::SameLine();
	if (ImGuiExt::KnobInt("Octaves", &m_Octaves, 1, 20, 0.35f))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Octaves");
		glUniform1ui(loc, (unsigned int) m_Octaves);
	}
	ImGuiExt::HelpMarker("Ocatves: Adds detail to the overall output by stacking 'n' layers of noise called octaves. "
						   "Each octave will be added on to the previous.\n\n"
						   "The behaviour of stacking octaves can be controlled by the Lacunarity and Persistence variables.");

	ImGui::SameLine();
	if (ImGuiExt::Knob("Lac", &m_Lacunarity, 0.0f, 10.0f))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Lacunarity");
		glUniform1f(loc, m_Lacunarity);
	}
	ImGuiExt::HelpMarker("Lacunarity: Determines the change in frequency/scale for each octave. For example, a Lacunarity of 2.0 "
						   "means that each octave of noise will have twice the detail of the previous.\n\n"
						   "A Lacunarity greater than 1.0 means that later octaves will have more detail/scale than the previous.\n\n"
						   "A Lacunarity less than 1.0 means that later octaves will have less detail/scale than the previous.\n\n"
						   "A Lacunarity equal to 1.0 means that later octaves will have the same detail/scale as the previous.");

	ImGui::SameLine();
	if (ImGuiExt::Knob("Pers", &m_Persistence, 0.0f, 10.0f))
	{
		GLint loc = glGetUniformLocation(m_Shader->GetRendererID(), "u_Persistence");
		glUniform1f(loc, m_Persistence);
	}
	ImGuiExt::HelpMarker("Persistence: Determines how much effect succeeding layers of octaves will have on the output noise. "
						   "For example, a Persistence of 0.5 means that each octave will only be added to the output at half the "
						   "strength of the previous.\n\n"
						   "A Persistence greater than 1.0 means that later octaves will have a stronger effect "
						   "than the previous.\n\n"
						   "A Persistence less than 1.0 means that later octaves will have a weaker effect "
						   "than the previous.\n\n"
						   "A Persistence equal to 1.0 means that all octaves will have an equal effect on the "
						   "output noise.");

	ImGui::End();

	m_DockSpace->End();

}