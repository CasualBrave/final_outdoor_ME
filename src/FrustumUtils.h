#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>

inline void extractFrustumPlanes(const glm::mat4& VP, glm::vec4 outPlanes[6]) {
	glm::vec4 col0 = glm::column(VP, 0);
	glm::vec4 col1 = glm::column(VP, 1);
	glm::vec4 col2 = glm::column(VP, 2);
	glm::vec4 col3 = glm::column(VP, 3);

	glm::vec4 planes[6] = {
		col3 + col0, // left
		col3 - col0, // right
		col3 + col1, // bottom
		col3 - col1, // top
		col3 + col2, // near
		col3 - col2  // far
	};

	for (int i = 0; i < 6; ++i) {
		glm::vec3 n = glm::vec3(planes[i]);
		float len = glm::length(n);
		if (len > 0.0001f) {
			planes[i] /= len;
		}
		outPlanes[i] = planes[i];
	}
}

inline glm::vec4 makePlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& insidePoint) {
	glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
	float d = -glm::dot(n, a);
	glm::vec4 plane(n, d);
	// ensure inside point yields positive distance
	if (glm::dot(plane, glm::vec4(insidePoint, 1.0f)) < 0.0f) {
		plane = -plane;
	}
	return plane;
}

// nearCorners and farCorners are each 4 points in world space, order:
// 0:(-1, 1), 1:(-1,-1), 2:( 1,-1), 3:( 1, 1) in camera clip XY.
inline void extractFrustumPlanesFromCorners(
	const glm::vec3 nearCorners[4],
	const glm::vec3 farCorners[4],
	glm::vec4 outPlanes[6]) {
	glm::vec3 center(0.0f);
	for (int i = 0; i < 4; ++i) {
		center += nearCorners[i] + farCorners[i];
	}
	center /= 8.0f;

	// planes: left, right, bottom, top, near, far
	outPlanes[0] = makePlane(nearCorners[0], nearCorners[1], farCorners[1], center); // left
	outPlanes[1] = makePlane(nearCorners[2], nearCorners[3], farCorners[3], center); // right
	outPlanes[2] = makePlane(nearCorners[1], nearCorners[2], farCorners[2], center); // bottom
	outPlanes[3] = makePlane(nearCorners[3], nearCorners[0], farCorners[0], center); // top
	outPlanes[4] = makePlane(nearCorners[0], nearCorners[3], nearCorners[2], center); // near
	outPlanes[5] = makePlane(farCorners[3], farCorners[0], farCorners[1], center);   // far
}
