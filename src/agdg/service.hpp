#pragma once

namespace agdg {
	class IService {
	public:
		virtual ~IService() {}

		virtual void Init() = 0;
		virtual void Start() = 0;
		virtual void Stop() = 0;
	};
}
