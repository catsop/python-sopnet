#include "SkeletonView.h"
#include <sg_gui/OpenGl.h>
#include <sg_gui/Colors.h>

SkeletonView::SkeletonView() :
	_sphere(10),
	_sphereScale(1.0) {}

void
SkeletonView::setSkeletons(std::shared_ptr<Skeletons> skeletons) {

	_skeletons = skeletons;

	updateRecording();
	send<sg_gui::ContentChanged>();
}

void
SkeletonView::onSignal(sg_gui::Draw& /*draw*/) {

	if (!_skeletons)
		return;

	draw();
}

void
SkeletonView::onSignal(sg_gui::QuerySize& signal) {

	if (!_skeletons)
		return;

	signal.setSize(_skeletons->getBoundingBox());
}

void
SkeletonView::onSignal(sg_gui::MouseDown& signal) {

	if (signal.modifiers & sg_gui::keys::ShiftDown) {

		if (signal.button == sg_gui::buttons::WheelUp) {

			_sphereScale *= 1.1;
			signal.processed = true;
		}

		if (signal.button == sg_gui::buttons::WheelDown) {

			_sphereScale = std::max(0.1, _sphereScale*(1.0/1.1));
			signal.processed = true;
		}

		updateRecording();
		send<sg_gui::ContentChanged>();
	}
}

void
SkeletonView::onSignal(SetSkeletons& signal) {

	setSkeletons(signal.getSkeletons());
}

void
SkeletonView::updateRecording() {

	sg_gui::OpenGl::Guard guard;

	startRecording();

	for (auto& p : *_skeletons) {

		unsigned char r, g, b;
		sg_gui::idToRgb(p.first, r, g, b);
		glColor4f(
				static_cast<float>(r)/200.0,
				static_cast<float>(g)/200.0,
				static_cast<float>(b)/200.0,
				1.0);

		drawSkeleton(p.second);
	}

	stopRecording();
}

void
SkeletonView::drawSkeleton(const Skeleton& skeleton) {

	glLineWidth(2.0);
	glEnable(GL_LINE_SMOOTH);

	GLfloat specular[] = {0.2, 0.2, 0.2, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	GLfloat shininess[] = {0.2, 0.2, 0.2, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	for (Skeleton::Graph::EdgeIt e(skeleton.graph()); e != lemon::INVALID; ++e) {

		Skeleton::Node u = skeleton.graph().u(e);
		Skeleton::Node v = skeleton.graph().v(e);

		Skeleton::Position pu = skeleton.positions()[u];
		Skeleton::Position pv = skeleton.positions()[v];

		util::point<float,3> ru;
		util::point<float,3> rv;
		skeleton.getRealLocation(pu, ru);
		skeleton.getRealLocation(pv, rv);

		float diameter = skeleton.diameters()[u];
		drawSphere(ru, diameter*_sphereScale);

		glBegin(GL_LINES);
		glVertex3d(ru.x(), ru.y(), ru.z());
		glVertex3d(rv.x(), rv.y(), rv.z());
		glEnd();
	}
}

void
SkeletonView::drawSphere(const util::point<float,3>& center, float diameter) {

	glPushMatrix();
	glTranslatef(center.x(), center.y(), center.z());
	glScalef(diameter, diameter, diameter);

	const std::vector<sg_gui::Triangle>& triangles = _sphere.getTriangles();

	glBegin(GL_TRIANGLES);
	for (const sg_gui::Triangle& triangle : triangles) {

		const sg_gui::Point3d&  v0 = _sphere.getVertex(triangle.v0);
		const sg_gui::Point3d&  v1 = _sphere.getVertex(triangle.v1);
		const sg_gui::Point3d&  v2 = _sphere.getVertex(triangle.v2);
		const sg_gui::Vector3d& n0 = _sphere.getNormal(triangle.v0);
		const sg_gui::Vector3d& n1 = _sphere.getNormal(triangle.v1);
		const sg_gui::Vector3d& n2 = _sphere.getNormal(triangle.v2);

		glNormal3f(n0.x(), n0.y(), n0.z()); glVertex3f(v0.x(), v0.y(), v0.z());
		glNormal3f(n1.x(), n1.y(), n1.z()); glVertex3f(v1.x(), v1.y(), v1.z());
		glNormal3f(n2.x(), n2.y(), n2.z()); glVertex3f(v2.x(), v2.y(), v2.z());
	}
	glEnd();

	glPopMatrix();
}
