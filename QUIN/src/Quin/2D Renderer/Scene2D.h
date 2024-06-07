#pragma once
//#include <qnpch.h>
#include "Renderer2D.h"

namespace Quin { namespace Renderer2D
{
	class QUIN_API Scene2D
	{
	public:
		Scene2D();
		~Scene2D();
	private:
		Renderer2D renderer;
	};
}}