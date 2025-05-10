FROM arm64v8/ros:noetic-robot

# update
RUN apt-get update && apt-get install -y && sudo apt install x11-apps -y

# qt
RUN sudo apt install libqt5multimedia5-plugins libqt5multimedia5 libqt5multimediawidgets5 libqt5multimediaquick5 libqt5multimedia5-plugins qtmultimedia5-dev -y

# ros
RUN sudo apt install ros-noetic-move-base-msgs ros-noetic-dwa-local-planner ros-noetic-rviz ros-noetic-turtlebot3 -y

# test.bash
COPY test.bash /root/test.bash
# Add test.bash content to .bashrc
RUN cat /root/test.bash >> /root/.bashrc

# entrypoint
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# entrypoint 설정
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]