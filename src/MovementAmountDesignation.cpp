#include "ros/ros.h"
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Int32.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include "T_Move.hpp"

using namespace std;

T_Move t_move;

class MovementAmountDesignation {
private:
	void callback(const std_msgs::Float64MultiArray::ConstPtr &msg);
	void setOdom(const nav_msgs::Odometry::ConstPtr &odom) {t_move.setOdometry(odom);}
	void Straight(double distance, double v);
	void Turn(double angle, double v);

public:
	MovementAmountDesignation();
	~MovementAmountDesignation();

	ros::NodeHandle n;
	ros::Subscriber amount;
	ros::Subscriber odom;
	ros::Publisher velocity;
	ros::Publisher signal;
	ros::Publisher move;

	double call_liner[2];
	double call_angle[2];

};

MovementAmountDesignation::MovementAmountDesignation() {

	printf("start class of 'MovementAmountDesignation'\n");

	this->odom = n.subscribe("/odom", 1000, &MovementAmountDesignation::setOdom, this);
	this->amount = n.subscribe("/move/amount", 1000, &MovementAmountDesignation::callback, this);
	this->move = n.advertise<geometry_msgs::Twist>("/mobile_base/commands/velocity", 1000);
	this->velocity = n.advertise<std_msgs::Float64MultiArray>("/move/velocity", 1000);
	this->signal = n.advertise<std_msgs::Int32 >("/move/signal", 1000);

	call_liner[0] = 0;
	call_liner[1] = 0;
	call_angle[0] = 0;
	call_angle[1] = 0;

}


MovementAmountDesignation::~MovementAmountDesignation() {

	printf("shutdown class of 'MovementAmountDesignation'\n");
}

void MovementAmountDesignation::callback(const std_msgs::Float64MultiArray::ConstPtr &msg) {

	//[0]:liner距離 , [1]:liner速度 , [2]:angle距離 , [3]:angle速度
	call_liner[0] = msg->data[0];
	call_liner[1] = msg->data[1];
	call_angle[0] = msg->data[2];
	call_angle[1] = msg->data[3];

	printf("move\n");

	t_move.resetAmount();

}

void MovementAmountDesignation::Straight(double distance, double v) {

}

void MovementAmountDesignation::Turn(double angle, double v) {

}

int main(int argc, char **argv) {

	ros::init(argc, argv, "MovementAmount");

	MovementAmountDesignation amount_move;

	t_move.init();

	t_move.resetAmount();

	ros::Rate loop_rate(8);

	while (ros::ok()) {

		t_move.update();
		t_move.pubTwist(amount_move.move, t_move.exeDistance(amount_move.call_liner[0], amount_move.call_liner[1], t_move.getAmount("straight")) , t_move.exeAngle(amount_move.call_angle[0], amount_move.call_angle[1], t_move.getAmount("turn")));
		t_move.pubSignal(amount_move.signal, t_move.getMoving("straight") || t_move.getMoving("turn") ? 1 : 0);
		loop_rate.sleep();

	}

	return 0;

}