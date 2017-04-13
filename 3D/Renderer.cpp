#include "Renderer.h"
#include "Engine.h"
#include "Time.h"
#include "QPC.h"
#include "SOIL.h"
#include "Camera.h"
#include "UILabel.h"
#include "AssetManager.h"
#include "World.h"
#include "Text.h"
#include "GPUMeshManager.h"

#include "Console.h"

#include "UIRectangleShape.h"
#include "UIConsole.h"
#include "UIButton.h"

const s32 MasterRenderer::validResolutionsRaw[2][NUM_VALID_RESOLUTIONS] =
{
	{ 1920, 1600, 1536, 1366, 1280, 1024, 960, 848},
	{ 1080, 900,  864,  768 , 720 , 576 , 540, 480},
};

const GLfloat quadVertices[] = {
	-1.0f,  1.0f,  0.0f, 1.0f,//TL
	1.0f,  1.0f,  1.0f, 1.0f,//TR
	1.0f, -1.0f,  1.0f, 0.0f,//BR

	1.0f, -1.0f,  1.0f, 0.0f,//BR
	-1.0f, -1.0f,  0.0f, 0.0f,//BL
	-1.0f,  1.0f,  0.0f, 1.0f//TL
};

GLfloat quadVerticesViewRays[] = {
	-1.0f,  1.0f,  0.0f, 1.0f, 999.f, 999.f,
	1.0f,  1.0f,  1.0f, 1.0f, 999.f, 999.f,
	1.0f, -1.0f,  1.0f, 0.0f, 999.f, 999.f,

	1.0f, -1.0f,  1.0f, 0.0f, 999.f, 999.f,
	-1.0f, -1.0f,  0.0f, 0.0f, 999.f, 999.f,
	-1.0f,  1.0f,  0.0f, 1.0f, 999.f, 999.f,
};

inline void MasterRenderer::initialiseScreenQuad()
{
	glGenVertexArrays(1, &vaoQuad);
	glGenBuffers(1, &vboQuad);

	glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	bilatBlurShader.use();
	auto locc = glGetUniformLocation(bilatBlurShader.getGLID(), "source");
	glUniform1i(locc, 0);

	glBindVertexArray(vaoQuad);
	glBindBuffer(GL_ARRAY_BUFFER, vboQuad);

	GLint posAttrib = glGetAttribLocation(bilatBlurShader.getGLID(), "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint texAttrib = glGetAttribLocation(bilatBlurShader.getGLID(), "texCoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

	ssaoShader.use();

	posAttrib = glGetAttribLocation(ssaoShader.getGLID(), "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	texAttrib = glGetAttribLocation(ssaoShader.getGLID(), "texCoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

	glGenVertexArrays(1, &vaoQuadViewRays);
	glGenBuffers(1, &vboQuadViewRays);

	glBindBuffer(GL_ARRAY_BUFFER, vboQuadViewRays);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerticesViewRays), quadVerticesViewRays, GL_STATIC_DRAW);

	auto program = shaderStore.getShader(String<32>("Standard"));
	program->use();

	glBindVertexArray(vaoQuadViewRays);
	glBindBuffer(GL_ARRAY_BUFFER, vboQuadViewRays);

	posAttrib = glGetAttribLocation(program->getGLID(), "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	texAttrib = glGetAttribLocation(program->getGLID(), "texCoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

	auto viewAttrib = glGetAttribLocation(program->getGLID(), "viewRay");
	glEnableVertexAttribArray(viewAttrib);
	glVertexAttribPointer(viewAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	program = shaderStore.getShader(String<32>("test"));

	glBindVertexArray(vaoQuadViewRays);
	glBindBuffer(GL_ARRAY_BUFFER, vboQuadViewRays);

	posAttrib = glGetAttribLocation(program->getGLID(), "p");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	texAttrib = glGetAttribLocation(program->getGLID(), "t");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

	viewAttrib = glGetAttribLocation(program->getGLID(), "v");
	glEnableVertexAttribArray(viewAttrib);
	glVertexAttribPointer(viewAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/*Engine::s.use();

	auto c = glGetUniformLocation(Engine::s(), "gDepth");
	glUniform1i(c, 0);
	auto cc = glGetUniformLocation(Engine::s(), "gNormal");
	glUniform1i(cc, 1);
	auto ccc = glGetUniformLocation(Engine::s(), "gAlbedoSpec");
	glUniform1i(ccc, 2);
	glUniform1i(glGetUniformLocation(Engine::s(), "ssaoTex"), 3);
	glUniform1i(glGetUniformLocation(Engine::s(), "shadow"), 8);

	Engine::s.stop();*/
}

inline void MasterRenderer::initialiseGBuffer()
{
	fboGBuffer.setResolution(rC.renderResolution);
	fboGBuffer.attachTexture(GL_RG16F, GL_RG, GL_HALF_FLOAT, GL_COLOR_ATTACHMENT0);//NORMAL
	fboGBuffer.attachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT1);//ALBEDO_SPEC
	fboGBuffer.attachTexture(GL_DEPTH_COMPONENT32F_NV, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
	fboGBuffer.attachTexture(GL_R32I, GL_RED_INTEGER, GL_INT, GL_COLOR_ATTACHMENT2);

	fboGBuffer.checkStatus();

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
}

inline void MasterRenderer::initialiseSSAOBuffer()
{
	fboSSAO.setResolution(rC.renderResolution);
	fboSSAO.attachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0, glm::fvec2(rC.ssaoScale));
	fboSSAO.checkStatus();

	fboSSAOBlur.setResolution(rC.renderResolution);
	fboSSAOBlur.attachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0, glm::fvec2(rC.ssaoScale));
	fboSSAOBlur.checkStatus();
}

inline void MasterRenderer::initialiseScreenFramebuffer()
{
	fboScreen.setResolution(rC.renderResolution);
	fboScreen.attachTexture(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
	fboScreen.checkStatus();
}

inline void MasterRenderer::initialiseSkybox()
{
	int width, height;
	unsigned char* image;

	std::string skyboxPath = "res/skybox/sky/";

	std::vector<std::string> faces;

	const char* paths[6] = 
	{
		{ "res/skybox/sky/right.png" },
		{ "res/skybox/sky/left.png" },
		{ "res/skybox/sky/top.png" },
		{ "res/skybox/sky/bottom.png" },
		{ "res/skybox/sky/front.png" },
		{ "res/skybox/sky/back.png" }
	};

	
	//sk.createFromFiles(&paths[0]);

	faces.push_back(skyboxPath + "right.png");
	faces.push_back(skyboxPath + "left.png");
	faces.push_back(skyboxPath + "top.png");
	faces.push_back(skyboxPath + "bottom.png");
	faces.push_back(skyboxPath + "front.png");
	faces.push_back(skyboxPath + "back.png");

	//glActiveTexture(GL_TEXTURE5);
	glGenTextures(1, &skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
	for (GLuint i = 0; i < 6; i++)
	{
		image = SOIL_load_image(faces[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

inline void MasterRenderer::initialiseLights()
{
	const int nr = 100;
	for (int i = 0; i < nr; ++i)
	{
		PointLightData light;
		light.position = glm::fvec3(float(Engine::rand() % 5000) - 10, float(Engine::rand() % 2) + 200, float(Engine::rand() % 2) + 1);
		//light.position = glm::fvec3(0.8, 0.1, 0.4);
		light.colour = glm::fvec3((Engine::rand() % 70) + 200, (Engine::rand() % 70) + 200, (Engine::rand() % 70) + 200);
		light.colour = glm::normalize(light.colour);  light.colour *= glm::fvec3(5.2f);
		light.linear = 0.0001f;
		light.quadratic = 0.0003f;
		light.updateRadius();
		lightManager.addPointLight(light);
	}

	//TODO: LAST NULL LIGHT FOR TILE CULLING OVERRUN

	lightManager.updateAllPointLights();

	const int nr2 = 0;
	for (int i = 0; i < nr2; ++i)
	{
		SpotLightData l;
		l.position = glm::fvec3(float(Engine::rand() % 2000) - 1000, float(Engine::rand() % 50) + 350, float(Engine::rand() % 2000) - 1000);
		auto cc = Engine::rand() % 3;
		switch (cc)
		{
		case(0) :
			l.colour = glm::fvec3(10, 0, 0);
			break;
		case(1) :
			l.colour = glm::fvec3(0, 10, 0);
			break;
		case(2) :
			l.colour = glm::fvec3(0, 0, 10);
			break;
		}
		l.colour = glm::fvec3(10, 10, 10);
		l.direction = glm::fvec3(-0.50, -1.0, 0.1);
		l.innerSpread = glm::cos(PI / 4.f);
		l.outerSpread = glm::cos(PI / 4.f);
		l.linear = 0.0001;
		l.quadratic = 0.0001;
		l.updateRadius();
		lightManager.addSpotLight(l);
	}

	lightManager.updateAllSpotLights();

	tileCullShader.use();
	tileCullShader.setPointLightCount(lightManager.pointLights.size());
	tileCullShader.setSpotLightCount(lightManager.spotLights.size());

	int plc = lightManager.pointLights.size();
	int slc = lightManager.spotLights.size();
	//tileCullShader->use();
	//tileCullShader->setUniform(String64("pointLightCount"), &plc);
	//tileCullShader->setUniform(String64("spotLightCount"), &slc);
	//tileCullShader->sendUniforms();
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glm::fvec3 ddir(-2, -2, -4);
	ddir = glm::normalize(ddir);

	DirectLightData dir = DirectLightData(ddir, glm::fvec3(0.1f, 0.1f, 0.125f));

	//shadowShader.use();

	//auto posAttrib = glGetAttribLocation(shadowShader(), "p");
	//glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	//glEnableVertexAttribArray(posAttrib);

	//fboLight.setResolution(glm::fvec2(shadRes) * frameScale * shadScale);
	//fboLight.attachTexture(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0, glm::fvec2(ssaoScale));
	//fboLight.attachTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
	//fboLight.attachTexture(GL_R32F, GL_RED, GL_FLOAT, GL_COLOR_ATTACHMENT0);
	//GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	//glDrawBuffers(1, attachments);
	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);
	//fboLight.checkStatus();
}

inline void MasterRenderer::initialiseSamplers()
{
	
	defaultSampler.initialiseDefaults();
	defaultSampler.setTextureWrapS(GL_REPEAT);
	defaultSampler.setTextureWrapT(GL_REPEAT);
	defaultSampler.setTextureMinFilter(GL_NEAREST_MIPMAP_LINEAR);
	defaultSampler.setTextureMagFilter(GL_LINEAR);
	defaultSampler.setTextureLODBias(-0.5);
	defaultSampler.setTextureCompareMode(GL_NONE);
	defaultSampler.setTextureAnisotropy(16);
	defaultSampler.bind(0);
	defaultSampler.bind(1);
	defaultSampler.bind(2);
	defaultSampler.bind(3);

	postSampler.setTextureWrapS(GL_MIRRORED_REPEAT);
	postSampler.setTextureWrapT(GL_MIRRORED_REPEAT);
	postSampler.setTextureMinFilter(GL_LINEAR);
	postSampler.setTextureMagFilter(GL_LINEAR);
	postSampler.setTextureCompareMode(GL_NONE);
	//postSampler.setTextureAnisotropy(16);
	
	//postSampler.bind(2);
	//postSampler.bind(3);
	postSampler.bind(4);
	postSampler.bind(5);

	cubeSampler.setTextureWrapS(GL_CLAMP_TO_EDGE);
	cubeSampler.setTextureWrapT(GL_CLAMP_TO_EDGE);
	cubeSampler.setTextureWrapR(GL_CLAMP_TO_EDGE);
	cubeSampler.setTextureMinFilter(GL_LINEAR);
	cubeSampler.setTextureMagFilter(GL_LINEAR);
	cubeSampler.bind(15);
	
	textSampler.setTextureWrapS(GL_CLAMP_TO_EDGE);
	textSampler.setTextureWrapT(GL_CLAMP_TO_EDGE);
	textSampler.setTextureWrapR(GL_CLAMP_TO_EDGE);
	textSampler.setTextureMinFilter(GL_LINEAR);
	textSampler.setTextureMagFilter(GL_LINEAR);
	textSampler.bind(12);

	auto h1 = Engine::assets.get2DTex("g")->getHandle(defaultSampler.getGLID());
	auto h2 = Engine::assets.get2DTex("gN")->getHandle(defaultSampler.getGLID());
	auto h3 = Engine::assets.get2DTex("gS")->getHandle(defaultSampler.getGLID());

	auto h4 = Engine::assets.get2DTex("oo")->getHandle(defaultSampler.getGLID());
	auto h5 = Engine::assets.get2DTex("pf")->getHandle(defaultSampler.getGLID());
	//auto h6 = Engine::assets.get2DTex("sp")->getHandle(defaultSampler.getGLID());

	auto h7 = Engine::assets.get2DTex("ooN")->getHandle(defaultSampler.getGLID());
	auto h8 = Engine::assets.get2DTex("pfN")->getHandle(defaultSampler.getGLID());
	//auto h9 = Engine::assets.get2DTex("spN")->getHandle(defaultSampler.getGLID());

	///TODO: make null texture (return when not found)
	///TODO: Auto make handles resident!!

	//Engine::h1 = glGetTextureSamplerHandleARB(Engine::t1.getGLID(), defaultSampler.getGLID());
	//Engine::h2 = glGetTextureSamplerHandleARB(Engine::t2.getGLID(), defaultSampler.getGLID());
	//Engine::h3 = glGetTextureSamplerHandleARB(Engine::t3.getGLID(), defaultSampler.getGLID());

	glMakeTextureHandleResidentARB(h1);
	glMakeTextureHandleResidentARB(h2);
	glMakeTextureHandleResidentARB(h3);

	glMakeTextureHandleResidentARB(h4);
	glMakeTextureHandleResidentARB(h5);
	//glMakeTextureHandleResidentARB(h6);

	glMakeTextureHandleResidentARB(h7);
	glMakeTextureHandleResidentARB(h8);
	//glMakeTextureHandleResidentARB(h9);
}

void MasterRenderer::initialiseRenderer(Window * pwin, Camera & cam)
{
	window = pwin;
	//MSAALevel = Msaalev;
	viewport.top = 0; viewport.left = 0; viewport.width = window->getSizeX(); viewport.height = window->getSizeY();
	rC.frameScale = 1.f;
	rC.renderResolution.x = viewport.width * rC.frameScale;
	rC.renderResolution.y = viewport.height * rC.frameScale;

	initialiseSamplers();
	//initialiseShaders();

	setActiveCam(cam);

	initialiseScreenQuad();
	initialiseSkybox();
	initialiseLights();

	initialiseFramebuffers();

	fboGBuffer.setClearDepth(0.f);
	//fboLight.setClearDepth(0.f);
	//glDepthRangedNV(1.f, -1.f);
	th.createFromStream(GL_RGBA32F, rC.renderResolution.x, rC.renderResolution.y, GL_RGBA, GL_FLOAT, NULL);

	//console = new UIConsole();
	//console->initOGL();
}

void MasterRenderer::initialiseShaders()
{
	shaderStore.loadShader(&tileCullShader);
	shaderStore.loadShader(&gBufferShader);
	shaderStore.loadShader(&bilatBlurShader);
	shaderStore.loadShader(&frustCullShader);
	shaderStore.loadShader(&ssaoShader);
	//ssaoShader.initialise();
	
	shaderStore.loadShader(ShaderProgram::VertFrag, String32("Shape2DShader"));
	shaderStore.loadShader(ShaderProgram::VertFrag, String32("Standard"));
	shaderStore.loadShader(ShaderProgram::VertFrag, String32("test"));
}

void MasterRenderer::initialiseFramebuffers()
{
	initialiseGBuffer();
	initialiseSSAOBuffer();
	initialiseScreenFramebuffer();
}

void MasterRenderer::reInitialiseFramebuffers()
{
	destroyFramebufferTextures();
	initialiseFramebuffers();
	th.release();
	th.createFromStream(GL_RGBA32F, rC.renderResolution.x, rC.renderResolution.y, GL_RGBA, GL_FLOAT, NULL);
}

void MasterRenderer::destroyFramebufferTextures()
{
	fboGBuffer.destroyAllAttachments();
	fboScreen.destroyAllAttachments();
	fboSSAO.destroyAllAttachments();
	fboSSAOBlur.destroyAllAttachments();
}

void MasterRenderer::setActiveCam(Camera & pCam)
{
	activeCam = &pCam;
	//TL 4  5
	//TR 10 11
	//BR 16 17

	//BR 22 23
	//BL 28 29
	//TL 34 35
	//ssaoShader.setProj(activeCam->proj, glm::ivec2(viewport.width, viewport.height));
	cameraProjUpdated();
	quadVerticesViewRays[4] = activeCam->viewRays2[3].x;
	quadVerticesViewRays[5] = activeCam->viewRays2[3].y;

	quadVerticesViewRays[10] = activeCam->viewRays2[2].x;
	quadVerticesViewRays[11] = activeCam->viewRays2[2].y;

	quadVerticesViewRays[16] = activeCam->viewRays2[1].x;
	quadVerticesViewRays[17] = activeCam->viewRays2[1].y;

	quadVerticesViewRays[22] = activeCam->viewRays2[1].x;
	quadVerticesViewRays[23] = activeCam->viewRays2[1].y;

	quadVerticesViewRays[28] = activeCam->viewRays2[0].x;
	quadVerticesViewRays[29] = activeCam->viewRays2[0].y;

	quadVerticesViewRays[34] = activeCam->viewRays2[3].x;
	quadVerticesViewRays[35] = activeCam->viewRays2[3].y;

	//tileCullShader.use();

	//auto loc = glGetUniformLocation(compShad(), "viewRays");
	//glUniform4fv(loc, 1, &activeCam->viewRaysDat[0]);
}

void MasterRenderer::cameraProjUpdated()
{
	gBufferShader.setProj(activeCam->proj);
	ssaoShader.setProj(activeCam->proj);
	ssaoShader.setViewport(glm::ivec2(viewport.width, viewport.height));
	frustCullShader.setProj(activeCam->proj);
}

void MasterRenderer::render()
{
	// *********************************************************** G-BUFFER PASS *********************************************************** //

	const int nr = 100;
	for (int i = 0; i < nr; ++i)
	{
		lightManager.pointLights[i].position.z = 1000.f * std::sin(Engine::programTime * 2.f) + 50.f;
	}

	lightManager.updateAllPointLights();

	//Frustum cull pass
	{
		glViewport(0, 0, rC.renderResolution.x, rC.renderResolution.y);
		fboGBuffer.bind();
		fboGBuffer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::ivec4 clearC(-1, -1, -1, -1);
		glClearBufferiv(GL_COLOR, GL_COLOR_ATTACHMENT2, &clearC.x);

		glDepthRangedNV(-1.f, 1.f);
		glClearDepth(1.f);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glCullFace(GL_BACK);

		if (rC.wireFrame)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		frustCullShader.use();

		auto pr = glm::transpose(activeCam->proj);

		glm::fvec4 p[4];
		p[0] = pr[3] - pr[0];
		p[1] = pr[3] + pr[0];
		p[2] = pr[3] - pr[1];
		p[3] = pr[3] + pr[1];

		for (int i = 0; i <= 3; ++i)
			p[i] = glm::normalize(p[i]);

		frustCullShader.setView(activeCam->view);
		frustCullShader.sendView();
		frustCullShader.setPlanes(glm::fmat4(p[0], p[1], p[2], p[3]));
		frustCullShader.sendPlanes();

		world->objectMetaBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 0);
		world->texHandleBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 1);
		world->drawIndirectBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
		world->drawCountBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
		world->instanceTransformsBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 4);
		world->visibleTransformsBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);
		world->instanceIDBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 6);

		glDispatchCompute(1, 1, 1);

		world->drawCountBuffer.getBufferSubData(0, sizeof(drawCount), &drawCount);
	}

	//GBuffer pass
	{
		gBufferShader.use();

		gBufferShader.setView(activeCam->view);
		gBufferShader.setCamPos(activeCam->pos);

		gBufferShader.sendView();
		gBufferShader.sendCamPos();

		gBufferShader.sendUniforms();

		const GPUMeshManager& mm = Engine::assets.meshManager;

		glBindVertexArray(mm.solidBatches.find(PNUU_T_S_N)->second.vaoID);

		world->texHandleBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
		world->visibleTransformsBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 4);
		world->drawIndirectBuffer.bind(GL_DRAW_INDIRECT_BUFFER);
		world->instanceIDBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 6);

		//glDrawArrays(GL_TRIANGLES, 0, 18);

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, drawCount, 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		if (rC.wireFrame)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	// *********************************************************** G-BUFFER PASS *********************************************************** //

	/*glViewport(0, 0, shadRes.x*shadScale, shadRes.y*shadScale);

	//fboDefault.bind();
	//fboDefault.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	fboLight.bind();
	fboLight.clear(GL_DEPTH_BUFFER_BIT);

	//glm::fmat4 projj = glm::ortho(-shadRes.x/2.f, shadRes.x/2.f, -shadRes.y/2.f, shadRes.y/2.f, 10.f, 1000.f);
	glm::fmat4 projj = glm::perspective(float(PI) / 8.f, 1.f, 10.f, 1000.f);
	//glm::fmat4 vview = glm::fmat4(0.937299, 0.219367, -0.2708289, 0, 0, 0.77707, 0.629414141, 0.0, 0.348525763, -0.589949369, 0.728347123, 0.0, -269.99057, -119.16075, -277.979919, 1.00);

	/*if (Engine::movingLight)
	{
	auto l = lightManager.getSpotLight(0);
	l->position = activeCam->pos - glm::fvec3(0, 10, 0);
	l->direction = activeCam->getDirectionVector();
	lightManager.updateSpotLight(0);
	lightView = glm::translate(activeCam->view, glm::fvec3(0, 10, 0));
	}*/

	/*//glm::fmat4 vview = glm::lookAt(ppos, LPOS - glm::fvec3(0, 0, 1), glm::fvec3(0, 1, 0));
	glm::fmat4 vview = lightView;

	shadowShader.use();
	glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(projj));
	glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(vview));

	glEnable(GL_DEPTH_TEST);

	for (auto itr = entities.begin(); itr != entities.end(); ++itr)
	{
		glBindVertexArray(itr->first->vao);
		glBindBuffer(GL_ARRAY_BUFFER, itr->first->vbo);
		for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
		{
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(*itr2));
			glDrawArrays(GL_TRIANGLES, 0, itr->first->data.numVert);
		}
	}

	glBindVertexArray(0);*/

	// *********************************************************** SSAO PASS *********************************************************** //
	
	//SSAO pass
	{
		glViewport(0, 0, rC.renderResolution.x * rC.ssaoScale, rC.renderResolution.y * rC.ssaoScale);

		//fboDefault.bind();
		//fboDefault.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		fboSSAO.bind();
		fboSSAO.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		ssaoShader.use();

		glBindVertexArray(vaoQuad);
		fboGBuffer.textureAttachments[2].bind(0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisable(GL_BLEND);
	}

	// *********************************************************** SSAO PASS *********************************************************** //

	// *********************************************************** BLUR PASS *********************************************************** //

	fboSSAOBlur.bind();
	fboSSAOBlur.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bilatBlurShader.use();

	glm::ivec2 axis(1, 0);
	bilatBlurShader.setAxis(axis);
	bilatBlurShader.sendUniforms();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboSSAO.textureAttachments[0].getGLID());
	glDrawArrays(GL_TRIANGLES, 0, 6);

	fboSSAO.bindDraw();
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	fboSSAOBlur.bindRead();
	glBlitFramebuffer(0, 0, rC.renderResolution.x * rC.ssaoScale, rC.renderResolution.y * rC.ssaoScale, 0, 0, rC.renderResolution.x * rC.ssaoScale, rC.renderResolution.y * rC.ssaoScale, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	fboSSAOBlur.bind();
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	axis = glm::ivec2(0, 1);
	bilatBlurShader.setAxis(axis);
	bilatBlurShader.sendUniforms();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	fboSSAO.bindDraw();
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	fboSSAOBlur.bindRead();
	glBlitFramebuffer(0, 0, rC.renderResolution.x * rC.ssaoScale, rC.renderResolution.y * rC.ssaoScale, 0, 0, rC.renderResolution.x * rC.ssaoScale, rC.renderResolution.y * rC.ssaoScale, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// *********************************************************** BLUR PASS *********************************************************** //

	tileCullShader.use();

	glBindTextureUnit(2, th.getGLID());
	glBindImageTexture(2, th.getGLID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(3, fboGBuffer.textureAttachments[0].getGLID());
	glBindImageTexture(3, fboGBuffer.textureAttachments[0].getGLID(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	fboGBuffer.textureAttachments[0].bindImage(3, GL_READ_ONLY);
	fboGBuffer.textureAttachments[1].bindImage(4, GL_READ_ONLY);
	//fboGBuffer.textureAttachments[2].bindImage(5, GL_READ_ONLY);
	fboGBuffer.textureAttachments[2].bind(5);
	fboGBuffer.textureAttachments[3].bindImage(7, GL_READ_ONLY);
	fboSSAO.textureAttachments[0].bindImage(6, GL_READ_ONLY);
	glActiveTexture(GL_TEXTURE15);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, sk.getGLID());
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	glm::fvec4 vrays;
	vrays.x = activeCam->viewRays2[2].x;
	vrays.y = activeCam->viewRays2[2].y;
	vrays.z = activeCam->viewRays2[0].x;
	vrays.w = activeCam->viewRays2[0].y;

	tileCullShader.setViewRays(vrays);
	tileCullShader.setView(activeCam->view);
	tileCullShader.setViewPos(activeCam->pos);

	tileCullShader.sendViewRays();
	tileCullShader.sendView();
	tileCullShader.sendViewPos();

	//tileCullShader.sendUniforms();

	lightManager.pointLightsBuffer.bindBase(0);
	lightManager.spotLightsBuffer.bindBase(1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glDispatchCompute(std::ceilf(rC.renderResolution.x / 16.f), std::ceilf(float(rC.renderResolution.y) / 16.f), 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	lightManager.pointLightsBuffer.unbind();
	lightManager.spotLightsBuffer.unbind();

	fboDefault.bind();
	fboDefault.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glBindVertexArray(vaoQuadViewRays);
	auto program = shaderStore.getShader(String<32>("test"));
	program->use();

	glUniform1i(glGetUniformLocation(program->getGLID(), "tex"), 2);
	th.bind(2);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	Engine::uiw->draw();

	Engine::console.draw();
	
	window->swapBuffers();
}

void MasterRenderer::bakeStaticLights()
{
	for (auto itr = lightManager.staticPointLights.begin(); itr != lightManager.staticPointLights.end(); ++itr)
	{

	}
}
