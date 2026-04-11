#ifndef PARTICLE_H
#define PARTICLE_H

class Particle {
private:
    double mass_;
    double x_;
    double y_;
    double vx_;
    double vy_;
    double ax_;
    double ay_;

public:
    Particle(double m, double x0, double y0);

    void setAcceleration(double ax, double ay);
    void addAcceleration(double ax, double ay);
    void setVelocity(double vx, double vy);

    void kick(double dt);
    void drift(double dt);

    double getMass() const;
    double getX() const;
    double getY() const;
    double getVx() const;
    double getVy() const;
    double getAx() const;
    double getAy() const;
};

#endif