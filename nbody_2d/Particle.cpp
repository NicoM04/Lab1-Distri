#include "Particle.h"

Particle::Particle(double m, double x0, double y0)
    : mass_(m), x_(x0), y_(y0), vx_(0.0), vy_(0.0), ax_(0.0), ay_(0.0) {}

void Particle::setAcceleration(double ax, double ay) {
    ax_ = ax;
    ay_ = ay;
}

void Particle::addAcceleration(double ax, double ay) {
    ax_ += ax;
    ay_ += ay;
}

void Particle::setVelocity(double vx, double vy) {
    vx_ = vx;
    vy_ = vy;
}

void Particle::kick(double dt) {
    vx_ += ax_ * dt;
    vy_ += ay_ * dt;
}

void Particle::drift(double dt) {
    x_ += vx_ * dt;
    y_ += vy_ * dt;
}

double Particle::getMass() const {
    return mass_;
}

double Particle::getX() const {
    return x_;
}

double Particle::getY() const {
    return y_;
}

double Particle::getVx() const {
    return vx_;
}

double Particle::getVy() const {
    return vy_;
}

double Particle::getAx() const {
    return ax_;
}

double Particle::getAy() const {
    return ay_;
}