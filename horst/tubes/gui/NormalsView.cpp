#include "NormalsView.h"
#include <sg_gui/OpenGl.h>

void
NormalsView::setMeshes(std::shared_ptr<sg_gui::Meshes> meshes) {

	_meshes = meshes;

	updateRecording();
}

void
NormalsView::onSignal(sg_gui::Draw& /*draw*/) {

	draw();
}

void
NormalsView::onSignal(sg_gui::QuerySize& signal) {

	if (!_meshes)
		return;

	signal.setSize(_meshes->getBoundingBox());
}

void
NormalsView::updateRecording() {

	sg_gui::OpenGl::Guard guard;

	startRecording();

	glEnable(GL_DEPTH_TEST);
	glColor3f(0, 0, 0);

	foreach (unsigned int id, _meshes->getMeshIds()) {

		const std::vector<sg_gui::Triangle>& triangles = _meshes->get(id)->getTriangles();

		foreach (const sg_gui::Triangle& triangle, triangles) {

			const sg_gui::Point3d&  v0 = _meshes->get(id)->getVertex(triangle.v0);
			const sg_gui::Point3d&  v1 = _meshes->get(id)->getVertex(triangle.v1);
			const sg_gui::Point3d&  v2 = _meshes->get(id)->getVertex(triangle.v2);
			const sg_gui::Vector3d& n0 = 10.0f*_meshes->get(id)->getNormal(triangle.v0);
			const sg_gui::Vector3d& n1 = 10.0f*_meshes->get(id)->getNormal(triangle.v1);
			const sg_gui::Vector3d& n2 = 10.0f*_meshes->get(id)->getNormal(triangle.v2);

			glBegin(GL_LINES);
			glVertex3f(v0.x(), v0.y(), v0.z()); glVertex3f(v0.x() + n0.x(), v0.y() + n0.y(), v0.z() + n0.z()); 
			glEnd();

			glBegin(GL_LINES);
			glVertex3f(v1.x(), v1.y(), v1.z()); glVertex3f(v1.x() + n1.x(), v1.y() + n1.y(), v1.z() + n1.z()); 
			glEnd();

			glBegin(GL_LINES);
			glVertex3f(v2.x(), v2.y(), v2.z()); glVertex3f(v2.x() + n2.x(), v2.y() + n2.y(), v2.z() + n2.z()); 
			glEnd();
		}
	}

	stopRecording();

	send<sg_gui::ContentChanged>();
}
