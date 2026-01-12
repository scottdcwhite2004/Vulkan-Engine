#include "CameraManager.h"

void CameraManager::addCamera(const Camera& camera)
{
	_cameras.push_back(camera);
	if (_cameras.size() == 1)
	{
		_currentCamera = camera;
	}
}

bool CameraManager::switchToCamera(const int& index)
{
	if (index < 0 || index > _cameras.size())
	{
		return false;
	}
	_currentCamera = _cameras[index];
	return true;
}

void CameraManager::rotateCurrentCamera(float yaw, float pitch)
{
	_currentCamera.rotateCamera(yaw, pitch);
}

void CameraManager::panCurrentCamera(float rightUnits, float forwardUnits, float upUnits)
{
	_currentCamera.panCamera(rightUnits, forwardUnits, upUnits);
}
