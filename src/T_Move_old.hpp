#include "ros/ros.h"
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int32.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

#ifndef   T_MOVE_HPP
#define   T_MOVE_HPP

using namespace std;

class T_Move {
public:
	T_Move();
	~T_Move();

	typedef struct {
		double x0 = 0;
		double y0 = 0;
		double elapsed = 0;
		double p1 = 0;
		double p2 = 0;
		double p3 = 0;
		double acceleration = 0;
		double v = 0;
		double initial_value = 0;
		double distance = 0;
	} VelocityProfile;

	typedef struct {
		double t0 = 0;
		double elapsed = 0;
		double p1 = 0;
		double p2 = 0;
		double p3 = 0;
		double acceleration = 0;
		double v = 0;
		double initial_value = 0;
		double deathbed_value = 0;
		double angle = 0;
	} AngleProfile;

	VelocityProfile v_data;
	AngleProfile a_data;

	void update() {ros::spinOnce();}

	double getPosition(string element);
	double getVelocity(string element);
	double getDistance(double x0, double y0);
	double getAngle(double a0);
	void setOdometry(const nav_msgs::Odometry::ConstPtr &odom);

	double toQuaternion_ang(double w, double z) {
		return abs((z > 0 ? 1 : 360) - this->toAngle(acos(w) * 2));
	}
	double toQuaternion_rad(double w, double z) {
		return acos(this->odom_data.angular_w) * (this->odom_data.angular_z > 0 ? 1 : -1) * 2;
	}

	double toAngle(double rad) {return rad * 180 / M_PI;}
	double toRadian(double angle) {return (angle * M_PI) / 180;}
	double sign(double A) {return  A == 0 ? 0 : A / abs(A);}

	//Distance
	void createStraight(double x0, double y0, double max_velocity, double distance, double a, double v0);
	double calcStraight(double point);
	void createTurn(double t0, double max_velocity, double angle, double a, double v0, double vn);
	double calcTurn(double angular_point);

	//Velocity
	double calcVelocityStraight(double k, double target);
	double calcVelocityTurn(double k, double target);

	void pubTwist(const ros::Publisher& pub, double v, double a);
	void pubSignal(const ros::Publisher& pub, int s);
	void pubVelocity(const ros::Publisher& pub, double v, double v_a, double a, double a_a);

private:

	double stackStraight = 0;
	double stackTurn = 0;

	typedef struct {
		double x;
		double y;
		double z;
		double angular_x;
		double angular_y;
		double angular_z;
		double angular_w;
		double linear_speed;
		double angular_speed;
	} Odometry;

	Odometry odom_data;
};

T_Move::T_Move() {
	printf("Start class of 'T_Move'\n");
}

T_Move::~T_Move() {
	printf("Shutdown class of 'T_Move'\n");
}

//現在の座標を取得
double T_Move::getPosition(string element) {

	if (element == "x") return this->odom_data.x;
	if (element == "y") return this->odom_data.y;
	if (element == "angle") return this->toQuaternion_ang(this->odom_data.angular_w, this->odom_data.angular_z);

	return -1;
}

//現在の速度を取得
double T_Move::getVelocity(string element) {

	if (element == "straight") {
		return abs(this->odom_data.linear_speed) > 0.01 ? this->odom_data.linear_speed : 0;
	}

	if (element == "turn") {
		return abs(this->odom_data.angular_speed) > 0.01 ? this->odom_data.angular_speed : 0;
	}

	return 1;
}

//距離の取得
double T_Move::getDistance(double x0, double y0) {

	return hypot(this->getPosition("x") - x0, this->getPosition("y") - y0);
}

//角度の取得
double T_Move::getAngle(double a0) {



	return 0;
}

void T_Move::createStraight(double x0, double y0, double max_velocity, double distance, double a, double v0) {

	this->v_data.v = max_velocity;
	this->v_data.acceleration = a;
	this->v_data.initial_value = v0;
	this->v_data.x0 = x0;
	this->v_data.y0 = y0;
	this->v_data.distance = distance;
	this->v_data.p3 = abs(distance);

	this->v_data.p1 = (this->v_data.v - this->v_data.initial_value) / this->v_data.acceleration;
	this->v_data.p2 = (this->v_data.acceleration * this->v_data.p3 - this->v_data.v) / this->v_data.acceleration;

	if (this->v_data.p1 + (this->v_data.p3 - this->v_data.p2) > this->v_data.p3) {
		this->v_data.p1 = this->v_data.p3 / 2;
		this->v_data.p2 = this->v_data.p3 / 2;
		printf("short\n");
	}

	printf("%f   %f   %f\n", v_data.p1, v_data.p2, v_data.p3 );

}

double T_Move::calcStraight(double point) {

	double result = 0.0;

	if (point >= 0 && point < this->v_data.p1) {
		printf("d1\n");
		result = this->v_data.acceleration * point + this->v_data.initial_value;
		printf("%f\n", result);
		return result;
	}

	if (point >= this->v_data.p1 && point < this->v_data.p2) {
		printf("d2\n");
		return this->v_data.v;
	}

	if (point >= this->v_data.p2 && point < this->v_data.p3) {
		result = -1 * this->v_data.acceleration * point + this->v_data.acceleration * this->v_data.p3;
		printf("d3\n");
		printf("%f\n", result);
		return result;
	}

	printf("d4\n");
	return 0.1;
}

void T_Move::createTurn(double t0, double max_velocity, double angle, double a, double v0, double vn) {

	this->a_data.v = max_velocity;
	this->a_data.acceleration = a;
	this->a_data.initial_value = v0;
	this->a_data.deathbed_value = vn;
	this->a_data.t0 = t0;
	this->a_data.angle = angle;
	this->a_data.p3 = abs(angle);

	this->a_data.p1 = (this->a_data.v - this->a_data.initial_value) / this->a_data.acceleration;
	this->a_data.p2 = ((this->a_data.initial_value + this->a_data.acceleration * this->a_data.p3) - this->a_data.v) / this->a_data.acceleration;

	if (this->a_data.p1 + (this->a_data.p3 - this->a_data.p2) > this->a_data.p3) {
		this->a_data.p1 = this->a_data.p3 / 2;
		this->a_data.p2 = this->a_data.p3 / 2;
		printf("short\n");
	}

	printf("%f   %f   %f\n", a_data.p1, a_data.p2, a_data.p3 );

}

double T_Move::calcTurn(double angular_point) {

	double result = 0.0;

	if (angular_point >= 0 && angular_point < this->a_data.p1) {
		printf("a_d1\n");
		result = this->a_data.acceleration * angular_point + this->a_data.initial_value;
		printf("%f\n", result);
		return result;
	}

	if (angular_point >= this->a_data.p1 && angular_point < this->a_data.p2) {
		printf("a_d2\n");
		return this->a_data.v;
	}

	if (angular_point >= this->a_data.p2 && angular_point < this->a_data.p3) {
		result = (-1 * this->a_data.acceleration * angular_point) + this->a_data.initial_value + (this->a_data.acceleration * this->a_data.p3);
		printf("a_d3\n");
		printf("result %f\n", result);
		return result;
	}

	printf("a_d4\n");
	return 0.45;
}

//速度のT制御
double T_Move::calcVelocityStraight(double d, double targetV) {

	if (targetV - this->stackStraight > 0) {

		this->stackStraight += d;

		if (this->stackStraight > targetV) this->stackStraight = targetV;

	} else {

		this->stackStraight -= d;

		if (this->stackStraight < targetV) this->stackStraight = targetV;

	}

	if (abs(this->stackStraight) < 0.01) this->stackStraight = 0;

	return this->stackStraight;
}

//角度のT制御
double T_Move::calcVelocityTurn(double d, double targetA) {

	if (targetA - this->stackTurn >= 0) {

		this->stackTurn += d;

		if (this->stackTurn > targetA) this->stackTurn = targetA;

	} else {

		this->stackTurn -= d;

		if (this->stackTurn < targetA) this->stackTurn = targetA;

	}

	if (abs(this->stackTurn) < 0.01) this->stackTurn = 0;

	return this->stackTurn;
}

//Twist送信
void T_Move::pubTwist(const ros::Publisher &pub, double v, double a) {

	geometry_msgs::Twist twist;

	twist.linear.x = v;
	twist.linear.y = 0;
	twist.linear.z = 0;
	twist.angular.x = 0;
	twist.angular.y = 0;
	twist.angular.z = a;

	pub.publish(twist);
}

//速度送信
void T_Move::pubVelocity(const ros::Publisher &pub, double v, double v_a, double a, double a_a) {

	std_msgs::Float64MultiArray info;

	info.data.clear();

	info.data.push_back(v);
	info.data.push_back(v_a);
	info.data.push_back(a);
	info.data.push_back(a_a);

	pub.publish(info);

}

//シグナル送信
void T_Move::pubSignal(const ros::Publisher &pub, int s) {

	std_msgs::Int32 signal;

	signal.data = s;

	pub.publish(signal);
}

//オドメトリ登録
void T_Move::setOdometry(const nav_msgs::Odometry::ConstPtr &odom) {

	this->odom_data.x = odom->pose.pose.position.x;
	this->odom_data.y = odom->pose.pose.position.y;
	this->odom_data.z = odom->pose.pose.position.z;
	this->odom_data.angular_x = odom->pose.pose.orientation.x;
	this->odom_data.angular_y = odom->pose.pose.orientation.y;
	this->odom_data.angular_z = odom->pose.pose.orientation.z;
	this->odom_data.angular_w = odom->pose.pose.orientation.w;
	this->odom_data.linear_speed = odom->twist.twist.linear.x;
	this->odom_data.angular_speed = odom->twist.twist.angular.z;

}

#endif