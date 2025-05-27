/**
 * Simple stop motion by Riley for SIT102
 *
 * This is a very basic stop motion program you can take photos through a webcam and then export them as .mp4
 * It makes use of OpenCV for the actual images side of things and splashkit for the interface.
 *
 * Everything is in this one file as I can only submit one file to ontrack otherwise this would definitly be broken up
 *
 */

// Libaries
#include <splashkit.h>
#include <opencv2/opencv.hpp>
#include <vector>
using std::to_string;

// Data structures

// Stores current video data such as the camera and frame that is displayed on screen
struct camera_data
{
    cv::VideoCapture camera;
    cv::Mat frame;
};

struct animation_frame
{
    cv::Mat image;             // OpenCV image
    string splash_bitmap_name; // Name of the thumnail loaded into splashkit
};

// Holds the actual information about the animation
struct animation_data
{
    int fps;                        // Frames per secound of animation
    vector<animation_frame> frames; // Frame data array
};

// The struct that holds all other information for the program
struct stopmotion_app
{
    bool onion_skin;
    bool playing;
    int frame_number; // The number of the currently selected frame
    int next_bitmap_id;
    unsigned int last_frame_time; // Keeps track of the time since the last frame draw used by the realtime player
    camera_data cam;
    animation_data ani;
};

stopmotion_app new_app()
{
    stopmotion_app app;
    app.onion_skin = false;
    app.playing = false;
    app.frame_number = -1;
    app.next_bitmap_id = 0;
    app.ani.fps = 5; // Intial fps
    app.last_frame_time = 0;
    return app;
}

/**
 * Draws a button which has a different image depending on it its state
 * @param on_bitmap bitmap of name to show when the buttons state is true
 * @param off_bitmap bitmap of name to show when the buttons state is false
 * @param location Location to draw bitmap given as the the location of the bitmap
 * @param state The current state of the button on draw
 *
 * @returns The new bool of the state of the button
 */
bool draw_image_button(string on_bitmap, string off_bitmap, string tooltip, point_2d location, bool state)
{
    bitmap current_bitmap;
    if (state)
    {
        current_bitmap = bitmap_named(on_bitmap);
    }
    else
    {
        current_bitmap = bitmap_named(off_bitmap);
    }

    // Check if the mouse is currently over the button
    if (bitmap_point_collision(current_bitmap, location, mouse_position()))
    {
        rectangle bounding = bitmap_bounding_rectangle(current_bitmap, location.x, location.y);
        // Draw hover box under image
        fill_rectangle(COLOR_GRAY, bounding);

        // Draw tooltip
        draw_text(tooltip, COLOR_BLACK, "montserrat", 15, bounding.x, bounding.y + bounding.height);

        // Check if the button is being clicked if so change the state
        if (mouse_clicked(LEFT_BUTTON))
        {
            state = !state;
        }
    }

    // Draw the bitmap
    draw_bitmap(current_bitmap, location.x, location.y);

    return state;
}

/**
 * Takes a open CV mat and converts it to a spashkit bitmap with a given size
 * !!It is very important that you free a image when its no longer needed this program is alread memory inficient as it is!!
 *
 * @param image Opencv mat
 * @param width Target width in pixels
 * @param height Target heiht in pixels
 * @param name A name for the loaded splashkit bitmap
 *
 * @returns A splashkit bitmap
 */
bitmap cv_to_splashkit(cv::Mat image, int width, int height, string name)
{
    // Check if a bitmap already exists with that name if so free it
    if (has_bitmap(name))
    {
        free_bitmap(bitmap_named(name));
    }
    // Resize the image
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(width, height));

    cv::imwrite("splashTemp.jpg", resized);
    return load_bitmap(name, "splashTemp.jpg");
}

void draw_timeline(const stopmotion_app &app) // Constant reference here!
{
    fill_rectangle(COLOR_GRAY, 400, 100, 100, 100);
    // Draw line to show where the next frame will be placed
    draw_line(COLOR_DARK_BLUE, 505, 90, 505, 210, option_line_width(3));

    // If there is currently no frame dont bother running the loop
    if (app.ani.frames.size() == 0)
    {
        return;
    }

    for (int i = -4; i <= 5; i++)
    {
        int index = app.frame_number + i;

        // If the requested frame is outside the range of frames skip this index
        if (index < 0 || index > app.ani.frames.size() - 1)
        {
            continue;
        }
        // fill_rectangle(COLOR_GRAY, 400 + 100 * i, 100, 100, 100);
        draw_bitmap(app.ani.frames[index].splash_bitmap_name, 405 + 100 * i, 105);
    }
}

void capture_frame(stopmotion_app &app)
{
    // Make a new frame struct for this new frame
    animation_frame new_frame;
    app.cam.camera >> new_frame.image; // Capture a frame from the camera and store it in the new frame.

    // Now that the image has been collected play the shutter sound
    play_sound_effect("camera_shutter");

    int new_frame_index = app.frame_number + 1; // Insert frame 1 ahead of the currently selected frame
    // Create a splashkit bitmap thumnail to show on the interface
    string bitmap_name = "thumnail-" + to_string(app.next_bitmap_id);
    app.next_bitmap_id++;
    cv_to_splashkit(new_frame.image, 90, 90, bitmap_name);
    new_frame.splash_bitmap_name = bitmap_name;

    // Save this new frame to the array
    app.ani.frames.insert(app.ani.frames.begin() + new_frame_index, new_frame);

    // Set the current frame to this new frame
    app.frame_number = new_frame_index;
}

void delete_frame(stopmotion_app &app)
{
    // Make sure that there is at least 1 frame to delete first
    if (app.ani.frames.size() > 0)
    {
        // Free the splashkit thumnail bitmap
        string bitmap_name = app.ani.frames[app.frame_number].splash_bitmap_name;
        free_bitmap(bitmap_named(bitmap_name));

        app.ani.frames.erase(app.ani.frames.begin() + app.frame_number);

        // Move the next frame to a place that makes sense for this delete
        if (app.frame_number > 0)
        {
            app.frame_number--;
        }
        else if (app.ani.frames.size() < 1)
        {
            app.frame_number = -1;
        }
    }
}

/**
 * Takes in the stopmotion_app struct and exports its current state to a video file.
 */
void export_video(const stopmotion_app &app, string filename)
{
    write_line("Saving video please wait!");
    cv::VideoWriter out(filename + ".mp4", cv::VideoWriter::fourcc('M', 'P', '4', 'V'), app.ani.fps, app.ani.frames[0].image.size());
    for (int i = 0; i < app.ani.frames.size(); i++)
    {
        out.write(app.ani.frames[i].image);
    }
    out.release();
    write_line("Done! Video saved");
}

void controller_interface(stopmotion_app &app)
{
    clear_screen(COLOR_WHITE);
    // draw_bitmap("wow", 100, 100);
    // Onion skin button
    app.onion_skin = draw_image_button("onion_on", "onion_off", "Onion skin", {820, 15}, app.onion_skin);

    // Delete button
    if (draw_image_button("bin_on", "bin_on", "Delete current frame", {10, 15}, false))
    {
        delete_frame(app);
    }

    // Playing button
    app.playing = draw_image_button("play_on", "play_off", "Play/Pause", {418, 220}, app.playing);

    // Frame step buttons
    if (draw_image_button("left_arrow", "left_arrow", "Back 1", {344, 220}, false))
    {
        if (app.frame_number > 0)
        {
            app.frame_number--;
        }
    }

    if (draw_image_button("right_arrow", "right_arrow", "Forward 1", {492, 220}, false))
    {
        if (app.frame_number < app.ani.frames.size() - 1)
        {
            app.frame_number++;
        }
    }

    if (draw_image_button("camera", "camera", "Capture Frame", {790, 220}, false))
    {
        capture_frame(app);
    }

    if (draw_image_button("video", "video", "Export video", {418, 5}, false))
    {
        export_video(app, "video");
    }

    // Draw fps selector
    draw_text("FPS:", COLOR_BLACK, "montserrat", 25, 0, 227);
    app.ani.fps = int(round(slider(app.ani.fps, 1, 30, {0, 250, 200, 50})));
    draw_interface();

    // Draw the frame timeline
    draw_timeline(app);

    refresh_screen(); // Run as fast a possible as I am also working with open CV in the same thread
}

/**
 * Displays prompt to user then reads integer from terminal
 */
int read_integer(string prompt)
{
    write(prompt);
    string input = read_line();
    while (!is_integer(input))
    {
        write_line("Input must be integer: ");
        write(prompt);
        input = read_line();
    }

    return convert_to_integer(input);
}

/**
 * Sets up all of the openCV stuff such as the webcam and display windows
 * @param cam A camera object to hold all the data that is needed
 */
void start_camera(camera_data &cam)
{
    // Get camera number from user
    write_line("Please enter the camera number you want to use. This will be 0 if you only have one camera connected");
    int cam_number = read_integer("Camera number: ");
    cv::VideoCapture camera(cam_number);

    if (!camera.isOpened())
    {
        write_line("Error! Couldnt find camera");
    }

    // Save newly created camera into struct.
    cam.camera = camera;

    // Open the two image viewing windows with the config that removes the tool bar from both
    cv::namedWindow("Live View", cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Animation view", cv::WINDOW_GUI_NORMAL | cv::WINDOW_AUTOSIZE);
}

/**
 * Looks after the recival of camera data and draws the images to their windows
 * The openCV dependent onion skin function is also applied here
 */
void camera_manager(stopmotion_app &app)
{
    app.cam.camera >> app.cam.frame; // Save the current camera view to the camera frame ready to drawn to the window.

    // Draw the live view with or without onion skin depending on options
    if (app.onion_skin && app.frame_number >= 0)
    {
        cv::Mat onion;
        cv::addWeighted(app.cam.frame, 0.7, app.ani.frames[app.frame_number].image, 0.3, 0.0, onion);
        cv::imshow("Live View", onion);
    }
    else
    {
        cv::imshow("Live View", app.cam.frame);
    }

    // Draw current frame view
    if (app.frame_number != -1)
    {
        cv::imshow("Animation view", app.ani.frames[app.frame_number].image);
    }
    else
    {
        cv::Mat black = cv::Mat::zeros(app.cam.frame.size(), app.cam.frame.type());
        // If no frame exists just show a black screen the same size as if a image was there
        cv::putText(black, "Start taking frames for them to show here!", cv::Point(0, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        cv::imshow("Animation view", black);
    }
}

/**
 * Returns true when it is time to draw a new frame.
 * This allows for the real time player to play at the right fps while not slowing down the interface
 */
bool is_ready_for_frame(int fps, unsigned int last_frame_time)
{
    unsigned int ms_per_frame = 1000 / fps;
    unsigned int now = current_ticks();
    unsigned int delta = now - last_frame_time;
    return delta >= ms_per_frame;
}

/**
 * Looks after the real time playing of the animation
 */
void player_manager(stopmotion_app &app)
{
    if (app.playing)
    {
        if (is_ready_for_frame(app.ani.fps, app.last_frame_time))
        {
            if (app.frame_number < app.ani.frames.size() - 1)
            {
                app.frame_number++;
            }
            else
            {
                app.frame_number = 0;
            }
            app.last_frame_time = current_ticks();
        }
    }
}

int main()
{
    // Load resources from bundle
    load_resource_bundle("UIBundle", "interface_graphics.txt");
    stopmotion_app app = new_app();
    start_camera(app.cam);

    // Open interface window
    open_window("Stopmotion Controller", 900, 300);

    while (!quit_requested())
    {
        process_events();
        controller_interface(app);
        player_manager(app);
        camera_manager(app);
    }
    return 0;
}