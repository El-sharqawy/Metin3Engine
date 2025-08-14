#include "Stdafx.h"
#include "Utils.h"

const std::string& GetObjectTypeName(const EObjectTypes& eObjectType)
{
	static const std::unordered_map<EObjectTypes, std::string> objectTypeNames = {
		{OBJECT_TYPE_NONE, "None"},
		{OBJECT_TYPE_STATIC, "Static"},
		{OBJECT_TYPE_DYNAMIC, "Dynamic"},
		{OBJECT_TYPE_KINEMATIC, "Player"},
	};

	auto it = objectTypeNames.find(eObjectType);
	if (it != objectTypeNames.end())
	{
		return it->second;
	}
	return objectTypeNames.at(OBJECT_TYPE_NONE); // Default to None if not found
}

const std::string& GetObjectTypeName(const CPhysicsObject* pPhysicsObject)
{
	static std::string defaultType = "None";
	if (!pPhysicsObject)
	{
		return defaultType;
	}

	defaultType = GetObjectTypeName(pPhysicsObject->GetType());
	return defaultType;
}