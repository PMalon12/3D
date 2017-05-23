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
	fboGBuffer.setResolution(config.renderResolution);
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
	fboSSAO.setResolution(config.renderResolution);
	fboSSAO.attachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0, glm::fvec2(config.ssaoScale));
	fboSSAO.checkStatus();

	fboSSAOBlur.setResolution(config.renderResolution);
	fboSSAOBlur.attachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0, glm::fvec2(config.ssaoScale));
	fboSSAOBlur.checkStatus();
}

inline void MasterRenderer::initialiseScreenFramebuffer()
{
	fboScreen.setResolution(config.renderResolution);
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
	const int nr = 0;
	for (int i = 0; i < nr; ++i)
	{
		auto& add = lightManager.addPointLight();
		add.setColour(glm::fvec3(2.f, 0.f, 2.f));
		add.setLinear(0.001f);
		add.setQuadratic(0.001f);
		add.setPosition(glm::fvec3(100.f, 100.f, 100.f));
		add.updateRadius();
		add.initTexture(shadowCubeSampler);
	}

	//TODO: LAST NULL LIGHT FOR TILE CULLING OVERRUN

	lightManager.updateAllPointLights();

	const int nr2 = 10;
	for (int i = 0; i < nr2; ++i)
	{
		auto& add = lightManager.addSpotLight();
		auto cc = Engine::rand() % 3;
		glm::fvec3 col(0.f);
		switch (cc)
		{
		case 0:
			col.x = 0.1;
			break;
		case 1:
			col.y = 0.1;
			break;
		case 2:
			col.z = 0.1;
			break;
		}
		add.setColour(col);
		add.setInnerSpread(glm::radians(10.f));
		add.setOuterSpread(glm::radians(30.f));
		add.setLinear(0.0001);
		add.setQuadratic(0.0001);
		add.setPosition(glm::fvec3(100.f, 100.f, 100.f));
		add.updateRadius();
		add.initTexture(shadowSampler);
	}

	lightManager.updateAllSpotLights();

	tileCullShader.use();
	tileCullShader.setPointLightCount(lightManager.pointLightsGPUData.size());
	tileCullShader.setSpotLightCount(lightManager.spotLights.size());

	int plc = lightManager.pointLightsGPUData.size();
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

	for (int i = 0; i < 4; ++i)
	{
		//fboLight[i].setResolution(glm::ivec2(shadowResolutions[i], shadowResolutions[i]));
		//fboLight[i].attachTexture(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
		//fboLight[i].checkStatus();
	}

	
	//fboLight.attachTexture(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0, glm::fvec2(ssaoScale));
	
	//fboLight.attachTexture(GL_R32F, GL_RED, GL_FLOAT, GL_COLOR_ATTACHMENT0);
	//GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	//glDrawBuffers(1, attachments);
	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);

	for (auto itr = lightManager.pointLights.begin(); itr != lightManager.pointLights.end(); ++itr)
	{
		auto handle = itr->shadowTex.getHandle(cubeSampler.getGLID());
		glMakeTextureHandleResidentARB(handle);
	}

	//shadow.createFromStream(1024, 1024, GL_DEPTH_COMPONENT32F_NV, GL_DEPTH_COMPONENT, GL_FLOAT);

	fboLight[0].bind();
	//fboLight[0].setResolution(glm::ivec2(512, 512));
	
	//fboLight[0].attachTexture(GL_DEPTH_COMPONENT32F_NV, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
	//fboLight[0].attachTexture(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
	//fboLight[0].attachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
	
	
	//fboLight[0].attachCubeTexture(GL_DEPTH_ATTACHMENT, shadow.getGLID());
	//fboLight[0].checkStatus();
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	//fboLight[0].checkStatus();
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

	shadowSampler.setTextureWrapS(GL_CLAMP_TO_BORDER);
	shadowSampler.setTextureWrapT(GL_CLAMP_TO_BORDER);
	shadowSampler.setTextureBorderColour(1.f, 1.f, 1.f, 1.f);
	shadowSampler.bind(14);

	shadowCubeSampler.setTextureMagFilter(GL_NEAREST);
	shadowCubeSampler.setTextureMinFilter(GL_NEAREST);
	shadowCubeSampler.setTextureWrapS(GL_CLAMP_TO_EDGE);
	shadowCubeSampler.setTextureWrapT(GL_CLAMP_TO_EDGE);
	shadowCubeSampler.setTextureWrapR(GL_CLAMP_TO_EDGE);
	shadowCubeSampler.bind(15);

	auto makeHandleResident = [&](GLTexture2D& tex) -> void {
		auto handle = tex.getHandle(defaultSampler.getGLID());
		glMakeTextureHandleResidentARB(handle);
	};

	///TODO: not all maps will use all textures in the texture store, keep track of what needs and what doesnt need to be resident
	for (auto itr = Engine::assets.getTextureList().begin(); itr != Engine::assets.getTextureList().end(); ++itr)
	{
		makeHandleResident(itr->second);
	}
}

void MasterRenderer::initialiseRenderer(Window * pwin, Camera & cam)
{
	window = pwin;
	//MSAALevel = Msaalev;
	viewport.top = 0; viewport.left = 0; viewport.width = window->getSizeX(); viewport.height = window->getSizeY();
	config.frameScale = 1.f;
	config.renderResolution.x = viewport.width * config.frameScale;
	config.renderResolution.y = viewport.height * config.frameScale;

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
	th.createFromStream(GL_RGBA32F, config.renderResolution.x, config.renderResolution.y, GL_RGBA, GL_FLOAT, NULL);

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
	shaderStore.loadShader(&gBufferShaderMultiTex);
	shaderStore.loadShader(&prepMultiTexShader);
	shaderStore.loadShader(&spotShadowPassShader);
	shaderStore.loadShader(&pointShadowPassShader);
	
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
	th.createFromStream(GL_RGBA32F, config.renderResolution.x, config.renderResolution.y, GL_RGBA, GL_FLOAT, NULL);
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
	gBufferShaderMultiTex.setProj(activeCam->proj);
	ssaoShader.setProj(activeCam->proj);
	ssaoShader.setViewport(glm::ivec2(viewport.width, viewport.height));
	frustCullShader.setProj(activeCam->proj);
}

void MasterRenderer::render()
{
	// *********************************************************** G-BUFFER PASS *********************************************************** //

	for (int i = 0; i < lightManager.spotLights.size(); ++i)
	{
		lightManager.spotLightsGPUData[i].position = glm::fvec3(std::cos(Engine::programTime*0.05*(i+1))*100.f, 50.f, std::sin(Engine::programTime*0.05*(i+1))*100.f);
		lightManager.spotLightsGPUData[i].direction = -glm::normalize(lightManager.spotLightsGPUData[i].position);
		lightManager.spotLights[i].updateProj();
		lightManager.spotLights[i].updateView();
		lightManager.spotLights[i].updateProjView();
	}

	for (int i = 0; i < lightManager.pointLights.size(); ++i)
	{
		lightManager.pointLightsGPUData[i].position.z = 100.f * std::sin(Engine::programTime * 0.8f);
		lightManager.pointLights[i].updateProj();
		lightManager.pointLights[i].updateView();
		lightManager.pointLights[i].updateProjView();
	}

	lightManager.updateAllPointLights();
	lightManager.updateAllSpotLights();

	//Frustum cull pass
	{
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

		world->objectMetaBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 0);
		world->texHandleBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 1);
		world->drawIndirectBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 2);
		world->drawCountBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 3);
		world->instanceTransformsBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 4);
		world->visibleTransformsBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 5);
		world->instanceIDBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 6);

		glDispatchCompute(1, 1, 1);

		world->drawCountBuffer[Regular].getBufferSubData(0, sizeof(drawCount[Regular]), &drawCount[Regular]);

		prepMultiTexShader.use();

		prepMultiTexShader.setView(activeCam->view);
		prepMultiTexShader.sendView();

		world->objectMetaBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 0);
		world->texHandleBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 1);
		world->drawIndirectBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 2);
		world->drawCountBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 3);
		world->instanceTransformsBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 4);
		world->visibleTransformsBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 5);
		world->instanceIDBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 6);

		prepMultiTexShader.setBaseID(drawCount[Regular]);

		prepMultiTexShader.sendUniforms();

		glDispatchCompute(1, 1, 1);

		world->drawCountBuffer[MultiTextured].getBufferSubData(0, sizeof(drawCount[MultiTextured]), &drawCount[MultiTextured]);
	}

	const GPUMeshManager& mm = Engine::assets.meshManager;

	//GBuffer pass
	{
		glViewport(0, 0, config.renderResolution.x, config.renderResolution.y);
		fboGBuffer.bind();

		glDepthRangedNV(-1.f, 1.f);

		fboGBuffer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::ivec4 clearC(-1, -1, -1, -1);
		glClearBufferiv(GL_COLOR, GL_COLOR_ATTACHMENT2, &clearC.x);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		//glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glCullFace(GL_BACK);

		glPolygonMode(GL_FRONT_AND_BACK, config.drawWireFrame ? GL_LINE : GL_FILL);

		gBufferShader.use();

		gBufferShader.setView(activeCam->view);
		gBufferShader.setCamPos(activeCam->pos);

		gBufferShader.sendView();
		gBufferShader.sendCamPos();

		gBufferShader.sendUniforms();

		glBindVertexArray(mm.solidBatches.find(Regular)->second.vaoID);

		world->texHandleBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 3);
		world->visibleTransformsBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 4);
		world->drawIndirectBuffer[Regular].bind(GL_DRAW_INDIRECT_BUFFER);
		world->instanceIDBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 6);

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, drawCount[Regular], 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		gBufferShaderMultiTex.use();

		gBufferShaderMultiTex.setView(activeCam->view);
		gBufferShaderMultiTex.setCamPos(activeCam->pos);

		gBufferShaderMultiTex.sendView();
		gBufferShaderMultiTex.sendCamPos();

		gBufferShaderMultiTex.sendUniforms();

		glBindVertexArray(mm.solidBatches.find(MultiTextured)->second.vaoID);

		world->texHandleBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 3);
		world->visibleTransformsBuffer[MultiTextured].bindBase(GL_SHADER_STORAGE_BUFFER, 4);
		world->drawIndirectBuffer[MultiTextured].bind(GL_DRAW_INDIRECT_BUFFER);
		world->instanceIDBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 6);

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, drawCount[MultiTextured], 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	
	}
	
	// *********************************************************** G-BUFFER PASS *********************************************************** //

	// *********************************************************** SHADOW PASS *********************************************************** //

	pointShadowPassShader.use();

	glDepthRange(0.f, 1.f);
	//glDepthRangedNV(-1.f, 1.f);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	for (auto itr = lightManager.pointLights.begin(); itr != lightManager.pointLights.end(); ++itr)
	{
		glViewport(0, 0, itr->shadowTex.getWidth(), itr->shadowTex.getHeight());

		fboLight[0].bind();
		fboLight[0].attachForeignCubeTexture(&itr->shadowTex, GL_DEPTH_ATTACHMENT);
		fboLight[0].checkStatus();

		glClear(GL_DEPTH_BUFFER_BIT);

		shadowMatrixBuffer.bufferData(sizeof(glm::fmat4) * 6, &itr->gpuData->projView[0][0], GL_STREAM_READ);
		shadowMatrixBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);

		pointShadowPassShader.setFarPlane(itr->gpuData->radius);
		pointShadowPassShader.setLightPos(itr->gpuData->position);
		pointShadowPassShader.sendUniforms();

		glBindVertexArray(mm.solidBatches.find(Shadow)->second.vaoID);
		world->drawIndirectBuffer[Shadow].bind(GL_DRAW_INDIRECT_BUFFER);
		world->visibleTransformsBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 1);

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, drawCount[Regular], 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	spotShadowPassShader.use();

	for (auto itr = lightManager.spotLights.begin(); itr != lightManager.spotLights.end(); ++itr)
	{
		glViewport(0, 0, itr->shadowTex.getWidth(), itr->shadowTex.getHeight());
		//glViewport(0, 0, 512, 512);

		fboLight[0].bind();
		fboLight[0].attachForeignTexture(&itr->shadowTex, GL_DEPTH_ATTACHMENT);
		fboLight[0].checkStatus();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		spotShadowPassShader.setProj(itr->proj);
		spotShadowPassShader.setView(itr->view);
		spotShadowPassShader.sendUniforms();

		glBindVertexArray(mm.solidBatches.find(Shadow)->second.vaoID);
		world->drawIndirectBuffer[Shadow].bind(GL_DRAW_INDIRECT_BUFFER);
		world->visibleTransformsBuffer[Regular].bindBase(GL_SHADER_STORAGE_BUFFER, 4);

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, drawCount[Regular], 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// *********************************************************** SHADOW PASS *********************************************************** //

	// *********************************************************** SSAO PASS *********************************************************** //
	
	//SSAO pass
	{
		glViewport(0, 0, config.renderResolution.x * config.ssaoScale, config.renderResolution.y * config.ssaoScale);

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
	glBlitFramebuffer(0, 0, config.renderResolution.x * config.ssaoScale, config.renderResolution.y * config.ssaoScale, 0, 0, config.renderResolution.x * config.ssaoScale, config.renderResolution.y * config.ssaoScale, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	fboSSAOBlur.bind();
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	axis = glm::ivec2(0, 1);
	bilatBlurShader.setAxis(axis);
	bilatBlurShader.sendUniforms();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	fboSSAO.bindDraw();
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	fboSSAOBlur.bindRead();
	glBlitFramebuffer(0, 0, config.renderResolution.x * config.ssaoScale, config.renderResolution.y * config.ssaoScale, 0, 0, config.renderResolution.x * config.ssaoScale, config.renderResolution.y * config.ssaoScale, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// *********************************************************** BLUR PASS *********************************************************** //

	tileCullShader.use();

	//glUniformMatrix4fv(20, 1, GL_FALSE, &lightManager.spotLightsGPUData[0].projView[0][0]);

	glBindTextureUnit(2, th.getGLID());
	glBindImageTexture(2, th.getGLID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(3, fboGBuffer.textureAttachments[0].getGLID());
	glBindImageTexture(3, fboGBuffer.textureAttachments[0].getGLID(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	fboGBuffer.textureAttachments[0].bindImage(3, GL_READ_ONLY);
	fboGBuffer.textureAttachments[1].bindImage(4, GL_READ_ONLY);
	fboGBuffer.textureAttachments[2].bind(5);
	fboGBuffer.textureAttachments[3].bindImage(7, GL_READ_ONLY);
	fboSSAO.textureAttachments[0].bindImage(6, GL_READ_ONLY);
	//fboLight[0].textureAttachments[0].bind(14);
	//lightManager.spotLights[0].shadowTex.bind(14);
	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
	
	//glBindTexture(GL_TEXTURE_CUBE_MAP, shadow.getGLID());

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

	tileCullShader.sendUniforms();

	lightManager.pointLightsBuffer.bindBase(0);
	lightManager.spotLightsBuffer.bindBase(1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glDispatchCompute(std::ceilf(config.renderResolution.x / 16.f), std::ceilf(float(config.renderResolution.y) / 16.f), 1);
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
	//for (auto itr = lightManager.staticPointLightsGPUData.begin(); itr != lightManager.staticPointLightsGPUData.end(); ++itr)
	//{
	//}
}


