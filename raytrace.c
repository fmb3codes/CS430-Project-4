//
//  raytrace.c
//  CS430 Project 4
//
//  Frankie Berry
//

// pre-processor directives
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>


// function prototypes 
void read_scene(char* filename); // master function for parsing the input json file

void raycasting(); // master function for raycasting and coloring pixels in the global image_data buffer

void write_image_data(char* output_file_name); // master function for writing image data from global image_data buffer to a ppm file (P6 in this case, as was recommended by the professor)

double sphere_intersection(double* Ro, double* Rd, double* C, double r); // checks whether or not there's a sphere intersection

double plane_intersection(double* Ro, double* Rd, double* C, double* n); // checks whether or not there's a plane intersection

int next_c(FILE* json); // wraps the getc() function and provides error checking and line number maintenance

void expect_c(FILE* json, int d); // checks that next character in file is d and displays error otherwise

void skip_ws(FILE* json); // skips over whitespace in json file

char* next_string(FILE* json); // parses next string from json file

double next_number(FILE* json); // parses next number from json file

double* next_vector(FILE* json); // parses next vector from json file

void normalize(double* v); // normalizes the given vector

double sqr(double v); // squares the given double value

double clamp (double value); // checks color range of value

void diffuse_calculation(double n[3], double l[3], double il[3], double kd[3], double* output); // performs diffuse color calculation

void specular_calculation(double n[3], double l[3], double il[3], double ks[3], double v[3], double r[3], double ns, double* output); // performs specular color calcuation


// object struct typedef'd as Object intended to hold any of the specified objects in the given scene (.json) file
typedef struct {
  int kind; // 0 = camera, 1 = sphere, 2 = plane, 3 = light
  double color[3]; // potentially remove here and add below
  union {
    struct {
      double width;
      double height;
    } camera;
    struct {
	  double diffuse_color[3];
	  double specular_color[3];
      double position[3];
      double radius;
    } sphere;
    struct {
	  double diffuse_color[3];
	  double specular_color[3];
	  double position[3];
	  double normal[3];
    } plane;
    struct {
	  int kind_light; // 0 = point light, 1 = spot light
	  double color[3];
	  double position[3];
	  double direction[3];
	  double radial_a2;
	  double radial_a1;
	  double radial_a0;
	  double angular_a0;
	  double theta;
    } light;
  };
} Object;

// radial/angular attenuation function prototypes placed after Object struct as they require it to be defined as a parameter
double frad(Object* light, double dl); // performs radial attenuation

double fang(Object* light, double direction[3], double theta); // performs angular attenuation


// header_data buffer which is intended to contain all relevant header information of ppm file
typedef struct header_data 
{
  char* file_format;
  char* file_comment;
  char* file_height;
  char* file_width;
  char* file_maxcolor;
} header_data;

// image_data buffer which is intended to hold a set of RGB pixels represented as unsigned chars
typedef struct image_data 
{
  unsigned char r, g, b;
} image_data;

int line = 1; // global int line variable to keep track of line in file

// global header_data buffer
header_data *header_buffer;

// global image_data buffer
image_data *image_buffer;

// global set of objects from json file
Object** objects;

double glob_width = 0; // global width, intended to store camera width
double glob_height = 0; // global height, intended to store camera height


int main(int argc, char** argv) 
{
	if(argc != 5) // checks for 5 arguments which includes the argv[0] path argument as well as the 4 required arguments of format [width height input.json output.ppm]
	{
		fprintf(stderr, "Error: Incorrect number of arguments; format should be -> [width height input.json output.ppm]\n");
		return -1;
	}
	
	int width = atoi(argv[1]); // the width of the scene
	int height = atoi(argv[2]); // the height of the scene
	char* input_file = argv[3]; // a .json file to read from
	char* output_file = argv[4]; // a .ppm file to output to
	
	// if statement which verifies that given length/width is not less than or equal to 0
	if(width <= 0 || height <= 0)
	{
		fprintf(stderr, "Error: Given width/height must not be less than or equal to 0\n");
		return -1;
	}
	
    // block of code which checks to make sure that user inputted a .json and .ppm file for both the input and output command line arguments
	char* temp_ptr_str;
	int input_length = strlen(input_file);
	int output_length = strlen(output_file);
	
	temp_ptr_str = input_file + (input_length - 5); // sets temp_ptr to be equal to the last 4 characters of the input_name, which should be .ppm
	if(strcmp(temp_ptr_str, ".json") != 0)
	{
		fprintf(stderr, "Error: Input file must be a .json file\n");
		return -1;
	}
	
	temp_ptr_str = output_file + (output_length - 4); // sets temp_ptr to be equal to the last 4 characters of the output_name, which should be .ppm
	if(strcmp(temp_ptr_str, ".ppm") != 0)
	{
		fprintf(stderr, "Error: Output file must be a .ppm file\n");
		return -1;
	}
	// end of .json/.ppm extension error checking	
  
  
	objects = malloc(sizeof(Object*)*129); // allocates memory for global object buffer to maximally account for 128 objects
  
	// block of code allocating memory to global header_buffer before its use
	header_buffer = (struct header_data*)malloc(sizeof(struct header_data)); 
	header_buffer->file_format = (char *)malloc(100);
	header_buffer->file_comment = (char *)malloc(1024);
	header_buffer->file_height = (char *)malloc(100);
	header_buffer->file_width = (char *)malloc(100);
	header_buffer->file_maxcolor = (char *)malloc(100);
  
	// block of code which hardcodes file format to be read out and also stores height/width from command line. Max color value is set at 255 as well
	strcpy(header_buffer->file_format, "P6");
	sprintf(header_buffer->file_height, "%d", height);
	sprintf(header_buffer->file_width, "%d", width);
	sprintf(header_buffer->file_maxcolor, "%d", 255);
  
  
	// image_buffer memory allocation here
	image_buffer = (image_data *)malloc(sizeof(image_data) * width * height + 1); // allocates memory for image based on width * height of image as given by command line
  
	read_scene(input_file); // parses json input file
	
	raycasting(); // executes raycasting based on information read in from json file in conjunction with the global image_buffer which handles the image pixels
 
	write_image_data(output_file); // writes "colored" pixels to ppm file after raycasting
  
	return 0;
}

// function which parses information from given json input file
void read_scene(char* filename) 
{
  int c;
  FILE* json = fopen(filename, "r");

  if (json == NULL) 
  {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  
  skip_ws(json);
  
  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);
  
  // Find the objects
   c = fgetc(json);
   if (c == ']')  // Quick check to see if there is an empty json file; displays an error accordingly
   { 
      fprintf(stderr, "Error: Empty Scene File.\n");
      fclose(json);
      exit(1);
    }
   ungetc(c, json); // ungets c after checking immediately for end of json file indicator (']')
   
   int i = 0; // iterator variable for objects

   // while loop intended to parse through all objects
   while (1) 
   {
    c = fgetc(json);
    if (c == '{') 
	{
	  // error-checking variables to make sure enough UNIQUE fields have been read-in for object after it has been parsed.
	  int camera_height_read = 0;
	  int camera_width_read = 0;
	  int sphere_diff_color_read = 0;
	  int sphere_spec_color_read = 0;
	  int sphere_position_read = 0;
	  int sphere_radius_read = 0;
	  int plane_diff_color_read = 0;
	  int plane_spec_color_read = 0;
	  int plane_position_read = 0;
	  int plane_normal_read = 0;
	  int light_color_read = 0;
	  int light_position_read = 0;
	  int light_direction_read = 0;
	  int light_rad2_read = 0;
	  int light_rad1_read = 0;
	  int light_rad0_read = 0;
	  int light_ang0_read = 0;
	  int light_theta_read = 0;
	  
      skip_ws(json);
    
      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) 
	  {
		fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
		exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      if (strcmp(value, "camera") == 0) // allocates memory for camera object and stores the "kind" as corresponding number
	  {
		  objects[i] = malloc(sizeof(Object));
		  objects[i]->kind = 0;
		  
		  
      } 
	  else if (strcmp(value, "sphere") == 0) // allocates memory for sphere object and stores the "kind" as corresponding number 
	  {
		  objects[i] = malloc(sizeof(Object));
		  objects[i]->kind = 1;
		  
		  
      } 
	  else if (strcmp(value, "plane") == 0) // allocates memory for plane object and stores the "kind" as corresponding number
	  {
		  objects[i] = malloc(sizeof(Object));
		  objects[i]->kind = 2;

		  
      } 
	  else if (strcmp(value, "light") == 0) // allocates memory for light object and stores the "kind" as corresponding number
	  {
		  objects[i] = malloc(sizeof(Object));
		  objects[i]->kind = 3;

		  
      } 
	  else // unknown object was read in so an error is displayed
	  {
		fprintf(stderr, "Error: Unknown object type, \"%s\", on line number %d.\n", value, line);
		exit(1);
      }

      skip_ws(json);

	 // while loop intended to parse through the fields a single object after guaranteed first field "type"
     while (1) 
	 {
	  // , }
	  c = next_c(json);
	  if (c == '}') 
	  {
	    // stop parsing this object
		// block of error checking code to first identify the current object that's finished parsing, and then checks to see if enough fields have been read in for that object.
		if(objects[i]->kind == 0)
		{
			if(camera_width_read != 1 || camera_height_read != 1)
			{
				fprintf(stderr, "Error: Object #%d (0-indexed) is a camera which should have two unique fields: width/height\n", i);
				exit(1);
			}
		}
		else if(objects[i]->kind == 1)
		{
			if(sphere_diff_color_read != 1 ||  sphere_spec_color_read != 1 ||  sphere_position_read != 1 || sphere_radius_read != 1)
			{
				fprintf(stderr, "Error: Object #%d (0-indexed) is a sphere which should have four unique fields: diffuse_color/specular_color/position/radius\n", i);
				exit(1);
			}
		}
		else if(objects[i]->kind == 2)
		{
			if(plane_diff_color_read != 1 ||  plane_spec_color_read != 1 || plane_position_read != 1 || plane_normal_read != 1)
			{
				fprintf(stderr, "Error: Object #%d (0-indexed) is a plane which should have four unique fields: diffuse_color/specular_color/position/normal\n", i);
				exit(1);
			}
		}
		else if(objects[i]->kind == 3)
		{
			if(light_color_read != 1 ||  light_position_read != 1)
			{
				fprintf(stderr, "Error: Object #%d (0-indexed) is a light which should have at least these 2 unique fields: color/position\n", i);
				exit(1);
			}
			else if(light_rad2_read == 1 && light_ang0_read == 0 && light_direction_read == 0) // point light should have at least rad2 and no angular-a0/direction
			{
				// block of code which defaults radial values if they weren't properly set
				if(light_rad1_read == 0)
					objects[i]->light.radial_a1 = 0;
				if(light_rad0_read == 0)
					objects[i]->light.radial_a0 = 0;
				// end of radial value check
				
				objects[i]->light.kind_light = 0;
			}
			else if(light_theta_read == 1 && light_ang0_read == 1 && light_direction_read == 1) // spot light
			{
				// block of code which defaults radial values if they weren't properly set
				if(light_rad2_read == 0)
					objects[i]->light.radial_a2 = 1;
				if(light_rad1_read == 0)
					objects[i]->light.radial_a1 = 0;
				if(light_rad0_read == 0)
					objects[i]->light.radial_a0 = 0;
				// end of radial value check
				
				// if theta was read in but the value is 0, then it's a point light
				if(objects[i]->light.theta == 0)
					objects[i]->light.kind_light = 0;
				// if theta was read in and the value is not 0, we know it's not less than 0, so it's a spot light
				if(objects[i]->light.theta != 0)
					objects[i]->light.kind_light = 1;
			}
			else // invalid number of fields read in for either type of light
			{
					fprintf(stderr, "Error: Object #%d (0-indexed) is a light which should be either a spotlight/pointlight. (ie must have fields: direction/angular-a0/theta or just radial-a2, respectively)\n", i);
					exit(1);
			}

		}
	    i++; // increments object iterator
		// resets all error-checking variables back to 0
		int camera_height_read = 0;
		int camera_width_read = 0;
		int sphere_diff_color_read = 0;
		int sphere_spec_color_read = 0;
		int sphere_position_read = 0;
		int sphere_radius_read = 0;
		int plane_diff_color_read = 0;
		int plane_spec_color_read = 0;
		int plane_position_read = 0;
		int plane_normal_read = 0;
		int light_color_read = 0;
		int light_position_read = 0;
		int light_direction_read = 0;
		int light_rad2_read = 0;
		int light_rad1_read = 0;
		int light_rad0_read = 0;
		int light_ang0_read = 0;
		int light_theta_read = 0;
	    break;
	  } 
	  else if (c == ',') 
	  {
	  // read another field
	  skip_ws(json);
	  char* key = next_string(json);
	  skip_ws(json);
	  expect_c(json, ':');
	  skip_ws(json);  
	  if ((strcmp(key, "width") == 0) || 
	      (strcmp(key, "height") == 0) || // evaluates if field is either a width/height/radius/radial-a2/radial-a1/radial-a0/angular-a0/theta
	      (strcmp(key, "radius") == 0) ||
		  (strcmp(key, "radial-a2") == 0) || 
	      (strcmp(key, "radial-a1") == 0) ||
		  (strcmp(key, "radial-a0") == 0) ||
		  (strcmp(key, "angular-a0") == 0) ||
		  (strcmp(key, "theta") == 0))
	  {
	    double value = next_number(json);
		if(strcmp(key, "width") == 0 && objects[i]->kind == 0) // evaluates only if key is width and current object is a camera
		{
			objects[i]->camera.width = value;
			glob_width = value; // stores camera width to prevent need to iterate through objects later
			camera_width_read++; // increments error checking variable for camera width field being read
		}
		else if(strcmp(key, "height") == 0 && objects[i]-> kind == 0) // evaluates only if key is height and current object is a camera
		{
			objects[i]->camera.height = value; 
			glob_height = value; // stores camera height to prevent need to iterate through objects later
			camera_height_read++; // increments error checking variable for camera height field being read
		}
		else if(strcmp(key, "radius") == 0 && objects[i]-> kind == 1) // evaluates only if key is radius and current object is a sphere
		{
			if(value <= 0) // error check to make sure a negative radius isn't read in from json file
			{
				fprintf(stderr, "Error: Sphere radius should not be less than or equal to 0. Violation found on line number %d.\n", line);
				exit(1);
			}
			objects[i]->sphere.radius = value;
			sphere_radius_read++; // increments error checking variable for sphere radius field being read
		}
		
		else if(strcmp(key, "radial-a2") == 0 && objects[i]-> kind == 3) // evaluates only if key is radial-a2 and current object is a light
		{
			if(value < 0) // error check to make sure a negative radial-a2 isn't read in from json file
			{
				fprintf(stderr, "Error: radial-a2 must be positive. Violation found on line number %d.\n", line);
				exit(1);
			}
			objects[i]->light.radial_a2 = value;
			light_rad2_read++; // increments error checking variable for radial-a2 field being read
		}
		else if(strcmp(key, "radial-a1") == 0 && objects[i]-> kind == 3) // evaluates only if key is radial-a1 and current object is a light
		{
			if(value < 0) // error check to make sure a negative radial-a1 isn't read in from json file
			{
				fprintf(stderr, "Error: radial-a1 must be positive. Violation found on line number %d.\n", line);
				exit(1);
			}
			objects[i]->light.radial_a1 = value;
			light_rad1_read++; // increments error checking variable for radial-a1 field being read
		}
		else if(strcmp(key, "radial-a0") == 0 && objects[i]-> kind == 3) // evaluates only if key is radial-a0 and current object is a light
		{
			if(value < 0) // error check to make sure a negative radial-a0 isn't read in from json file
			{
				fprintf(stderr, "Error: radial-a0 must be positive. Violation found on line number %d.\n", line);
				exit(1);
			}
			objects[i]->light.radial_a0 = value;
			light_rad0_read++; // increments error checking variable for radial-a0 field being read
		}
		else if(strcmp(key, "angular-a0") == 0 && objects[i]-> kind == 3) // evaluates only if key is angular-a0 and current object is a light
		{
			if(value < 0) // error check to make sure a negative angular-a0 isn't read in from json file
			{
				fprintf(stderr, "Error: angular-a0 must be positive. Violation found on line number %d.\n", line);
				exit(1);
			}
			objects[i]->light.angular_a0 = value;
			light_ang0_read++; // increments error checking variable for angular-a0 field being read
		}
		else if(strcmp(key, "theta") == 0 && objects[i]-> kind == 3) // evaluates only if key is angular-a0 and current object is a light
		{
			if(value < 0) // error check to make sure a negative theta isn't read in from json file
			{
				fprintf(stderr, "Error: theta must be greater than or equal to 0. Violation found on line number %d.\n", line);
				exit(1);
			}
			objects[i]->light.theta = value;
			light_theta_read++; // increments error checking variable for theta field being read
		}			
		else // after key was identified as width/height/radius/radial-a2/radial-a1/radial-a0/angular-a0/theta, object type is unknown so display an error
		{
			fprintf(stderr, "Error: Only cameras should have width/height, spheres have radius, and lights have radial-a2/radial-a1/radial-a0/angular-a0/theta. Violation found on line number %d.\n", line);
            exit(1);
		}
		
	  } 
	  else if ((strcmp(key, "color") == 0) ||
		     (strcmp(key, "diffuse_color") == 0) ||
		     (strcmp(key, "specular_color") == 0) ||
		     (strcmp(key, "position") == 0) || // evaluates if field is either a color/diffuse_color/specular_color/position/normal/direction
		     (strcmp(key, "normal") == 0) ||
			 (strcmp(key, "direction") == 0))
	  { 
	    double* value = next_vector(json);
		if(strcmp(key, "color") == 0 && objects[i]->kind == 3) // evaluates only if key is color and current object is a light
		{
			int j = 0; // iterator variable for error-checking
			for(j = 0; j < 3; j+=1) // error checking for loop to make sure color values from object are less than 0
			{
				if(value[j] < 0) // assuming color value must be less than 0
				{
					fprintf(stderr, "Error: Light color values must not be less than 0. Violation found on line number %d.\n", line); // CHECK ERROR CHECKING ON THIS
					exit(1);
				}
			}
			objects[i]->light.color[0] = value[0];
			objects[i]->light.color[1] = value[1]; // assigns color values from value vector to current object 
			objects[i]->light.color[2] = value[2];
			light_color_read++;
			
		}
		else if((strcmp(key, "diffuse_color") == 0 && objects[i]->kind == 1) || (strcmp(key, "diffuse_color") == 0 && objects[i]->kind == 2)) // evaluates only if key is diffuse_color and current object is a sphere or plane
		{
			int j = 0; // iterator variable for error-checking
			for(j = 0; j < 3; j+=1) // error checking for loop to make sure color values from object are between 0 and 1 (inclusive)
			{
				if(value[j] < 0 || value[j] > 1) // assuming color value must be between 0 and 1 (inclusive) due to example json file given along with corresponding ppm output file indicating so
				{
					fprintf(stderr, "Error: Sphere and Plane color values should be between 0 and 1 (inclusive). Violation found on line number %d.\n", line);
					exit(1);
				}
			}
			if(objects[i]->kind == 1)
			{
				objects[i]->sphere.diffuse_color[0] = value[0];
				objects[i]->sphere.diffuse_color[1] = value[1]; // assigns color values from value vector to current object 
				objects[i]->sphere.diffuse_color[2] = value[2];
				sphere_diff_color_read++; // increments error checking variable for sphere color field being read
			}
			else if(objects[i]->kind == 2)
			{
				objects[i]->plane.diffuse_color[0] = value[0];
				objects[i]->plane.diffuse_color[1] = value[1]; // assigns color values from value vector to current object 
				objects[i]->plane.diffuse_color[2] = value[2];
				plane_diff_color_read++; // increments error checking variable for plane color field being read
			}
		}
		else if((strcmp(key, "specular_color") == 0 && objects[i]->kind == 1) || (strcmp(key, "specular_color") == 0 && objects[i]->kind == 2)) // evaluates only if key is diffuse_color and current object is a sphere or plane
		{
			int j = 0; // iterator variable for error-checking
			for(j = 0; j < 3; j+=1) // error checking for loop to make sure color values from object are between 0 and 1 (inclusive)
			{
				if(value[j] < 0 || value[j] > 1) // assuming color value must be between 0 and 1 (inclusive) due to example json file given along with corresponding ppm output file indicating so
				{
					fprintf(stderr, "Error: Sphere and Plane color values should be between 0 and 1 (inclusive). Violation found on line number %d.\n", line);
					exit(1);
				}
			}
			if(objects[i]->kind == 1)
			{
				objects[i]->sphere.specular_color[0] = value[0];
				objects[i]->sphere.specular_color[1] = value[1]; // assigns color values from value vector to current object 
				objects[i]->sphere.specular_color[2] = value[2];
				sphere_spec_color_read++; // increments error checking variable for sphere color field being read
			}
			else if(objects[i]->kind == 2)
			{
				objects[i]->plane.specular_color[0] = value[0];
				objects[i]->plane.specular_color[1] = value[1]; // assigns color values from value vector to current object 
				objects[i]->plane.specular_color[2] = value[2];
				plane_spec_color_read++; // increments error checking variable for plane color field being read
			}
		}
		else if((strcmp(key, "position") == 0 && objects[i]->kind == 1) || ((strcmp(key, "position") == 0 && objects[i]->kind == 2)) || ((strcmp(key, "position") == 0 && objects[i]->kind == 3))) // evaluates only if key is position and current object is a sphere or plane or light
		{
			if(objects[i]->kind == 1)
			{
				objects[i]->sphere.position[0] = value[0];
				objects[i]->sphere.position[1] = -value[1]; // assigns position values from value vector to current sphere object 
				objects[i]->sphere.position[2] = value[2];
				sphere_position_read++; // increments error checking variable for sphere position field being read
			}
			else if(objects[i]->kind == 2)
			{
				objects[i]->plane.position[0] = value[0];
				objects[i]->plane.position[1] = value[1]; // assigns position values from value vector to current plane object 
				objects[i]->plane.position[2] = value[2];
				plane_position_read++; // increments error checking variable for plane position field being read
			}
			else if(objects[i]->kind == 3)
			{
				objects[i]->light.position[0] = value[0];
				objects[i]->light.position[1] = -value[1]; // assigns position values from value vector to current light object 
				objects[i]->light.position[2] = value[2];
				light_position_read++; // increments error checking variable for light position field being read
			}
			else // Evaluates if there is a mismatched object field with sphere/plane/light and position, but should never happen
			{
				fprintf(stderr, "Error: Mismatched object field \"%s\", on line %d.\n", key, line);
				exit(1);
			}
		}
		else if(strcmp(key, "direction") == 0 && objects[i]->kind == 3) // evaluates only if key is direction and current object is a light
		{
			objects[i]->light.direction[0] = value[0];
			objects[i]->light.direction[1] = -value[1]; // assigns direction values from value vector to current light object 
			objects[i]->light.direction[2] = value[2];
			light_direction_read++; // increments error checking variable for light direction field being read
		}
		else if(strcmp(key, "normal") == 0 && objects[i]->kind == 2) // evaluates only if key is normal and current object is a plane
		{
			objects[i]->plane.normal[0] = value[0];
			objects[i]->plane.normal[1] = value[1]; // assigns normal values from value vector to current plane object 
			objects[i]->plane.normal[2] = value[2];
			plane_normal_read++; // increments error checking variable for plane normal field being read
		}
		else // after key was identified as color/diffuse_color/specular_color/position/direction/normal, object type is unknown so display an error
		{
			fprintf(stderr, "Error: Only spheres/planes/lights have positions, spheres and planes have specular/diffuse colors, lights have colors and direction, and only planes have a normal. Violation found on line number %d.\n", line);
            exit(1);
		}
	  } 
	  else // unknown field was read in so display an error
	  { 
	    fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n", key, line);
		exit(1);
	  }
	  skip_ws(json);
	  } 
	  else  // expecting either a new field or the end of an object so display an error
	  {
		fprintf(stderr, "Error: Unexpected value on line %d. Expected either ',' or '}' to indicate next field or end of object.\n", line);
		exit(1);
	  }
     }
      skip_ws(json);
      c = next_c(json);
      if (c == ',') // Should be followed by another object
	  { 
		// noop
		skip_ws(json);
      } 
	  else if (c == ']')  // reached end of json file
	  {
			objects[i] = NULL; // null-terminate after last object
			fclose(json);
			return;
      } 
	  else // finished parsing an object and a comma or hard bracket was expect to indicate a new object/end of object list, so display error
	  {
			fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
			exit(1);
      }
    }
    else // didn't find end of file or the beginning of an object
	{
		fprintf(stderr, "Error: Expecting '{' or ']' on line %d.\n", line);
		exit(1);
	}
  }
}

// function which handles raycasting for objects read in from json file
void raycasting() 
{
		image_data current_pixel; // temp image_data struct which will hold RGB pixels
		image_data* temp_ptr = image_buffer; // temp ptr to image_data struct which will be used to navigate through global buffer
		current_pixel.r = 0;
		current_pixel.g = 0; // initializes current pixel RGB values to 0 (black)
		current_pixel.b = 0;
	
		
		// block of code to create/facilitate a new Object array which will store only light objects for later use
		Object** lights;
		lights = malloc(sizeof(Object) * 129);
		int light_counter = 0;
		
		for(int l = 0; objects[l] != 0; l+=1)
		{
			if(objects[l]->kind == 3)
			{
				lights[light_counter++] = objects[l];
			}
		}
		lights[light_counter] = NULL;
		// end of block of code for creating/filling new lights array
		
		
		// sets cx and cy values of camera (assumed to be at 0, 0)
		double cx = 0;
		double cy = 0;
		
		// sets width and height of image based on given width/height from command line that was previously stored in the global header buffer
		int M = atoi(header_buffer->file_height); 
		int N = atoi(header_buffer->file_width); 
		
		// sets pixheight and pixwidth using M and N declared above as well as camera height/width stored in global variables glob_height/glob_width during json parsing 
		double pixheight = glob_height / M;
		double pixwidth = glob_width / N;
				
		double Ro[3] = {0, 0, 0}; // Initializes origin ray to the assumed 0, 0, 0 position
		double Rd[3] = {0, 0, 0}; // Initializes direction of ray to 0, 0, 0 which will be changed
		double ray[3] = {0, 0, 1}; // Initializes temporary ray with 0, 0 for the x and y values and 1 for the assumed z value position
		
		for (int y = 0; y < M; y += 1) {
			ray[1] = (cy - (glob_height/2) + pixheight * (y + 0.5)); // calculates y-position of ray and stores accordingly
			for (int x = 0; x < N; x += 1) {
				ray[0] = cx - (glob_width/2) + pixwidth * (x + 0.5); // calculates x-position of ray and stores accordingly
				// stores the calculated ray values along with the assumed z value of 1 into the Rd vector
				Rd[0] = ray[0];
				Rd[1] = ray[1];
				Rd[2] = ray[2];
				normalize(Rd); // normalizes the Rd vector
				
				double color[3] = {0, 0, 0};

					double best_t = INFINITY;
					int best_i = 0;
					//double t = 0;
					for (int i=0; objects[i] != 0; i += 1) { // iterates through objects
						double t = 0; // sets t value to 0 before evaluating objects

						switch(objects[i]->kind) { // switch statement used to check object type and intersection information accordingly
						case 0: // object is a camera so break
							break; 
						case 1: // object is a sphere so calculate sphere intersection
							t = sphere_intersection(Ro, Rd,
														objects[i]->sphere.position,
														objects[i]->sphere.radius);	
							
							break;
						case 2: // object is a plane so calculate plane intersection
							t = plane_intersection(Ro, Rd,
														objects[i]->plane.position,
														objects[i]->plane.normal);
														
							break;
						case 3: // object is a light so break
							break; 
						default:
							fprintf(stderr, "Error: Unrecognized object.\n"); // Error in case siwtch doesn't evaluate as a known object but should never happen
							exit(1);
						}
						if (t > 0 && t < best_t) // stores best_t if there's a dominant intersection. Also stores best_i to record current object index
						{
							best_t = t; 
							best_i = i;
						}
					}
					
					if (best_t > 0 && best_t != INFINITY) {
						double Ron[3] = {0, 0, 0}; // Initializes new origin ray to the assumed 0, 0, 0 position
						double Rdn[3] = {0, 0, 0}; // Initializes new direction of ray to 0, 0, 0 which will be changed
						
						Ron[0] = best_t * Rd[0] + Ro[0];
						Ron[1] = best_t * Rd[1] + Ro[1]; // sets Ron using previously calculated object intersection
						Ron[2] = best_t * Rd[2] + Ro[2];
						
						for(int j =  0; lights[j] != 0; j+=1) // new outer for loop which iterates for every light in the lights array
						{							
							Rdn[0] = lights[j]->light.position[0] - Ron[0];
							Rdn[1] = lights[j]->light.position[1] - Ron[1]; // sets Rdn using current light's position and the previously calculated Ron vector
							Rdn[2] = lights[j]->light.position[2] - Ron[2];
							
							double distance_to_light = sqrt(sqr(Rdn[0]) + sqr(Rdn[1]) + sqr(Rdn[2]));  // calculates the distance to the light using the Rdn vector
							normalize(Rdn);				
							
							
							double new_best_t = INFINITY; // creates new intersection t variable
							int best_s = -1; // initializes variable to act as indication of "closest shadow object"
							for(int k = 0; objects[k] != 0; k+=1) // iterates through objects
							{
								if(objects[best_i] == objects[k]) // checks to make sure that the object checked isn't the previously calculated closest object
									continue;
								double t = 0;  // sets t value to 0 before evaluating objects
								
								switch(objects[k]->kind) { // switch statement used to check object type and intersection information accordingly
								case 0: // object is a camera so break
									break; 
								case 1: // object is a sphere so calculate sphere intersection
									t = sphere_intersection(Ron, Rdn,
																objects[k]->sphere.position,
																objects[k]->sphere.radius);	
							
									break;
								case 2: // object is a plane so calculate plane intersection
									t = plane_intersection(Ron, Rdn,
																objects[k]->plane.position,
																objects[k]->plane.normal);
														
									break;
								case 3: // object is a light so break
									break;
								default:
									fprintf(stderr, "Error: Unrecognized object.\n"); // Error in case siwtch doesn't evaluate as a known object but should never happen
									exit(1);
								}
								if (t > distance_to_light)
								{
									continue;
								}
								if (t > 0 && t < new_best_t) // stores new_best_t if there's a dominant shadow intersection. Also stores best_s to record current object index
								{
									new_best_t = t; 
									best_s = k;
								}
							}
							if(best_s == -1) // no closest shadow was found since best_s was unmodified (would never be set to -1 otherwise)
							{ 					
								// initializes necessary n, l, r, v, and nv vectors as well as diffuse and specular vectors
								double n[3]; 
								double l[3];
								double r[3]; // reflection of l
								double v[3];
								double nv[3];
								double diffuse[3] = {0, 0, 0};
								double specular[3] = {0, 0, 0};
								
								if(objects[best_i]->kind == 1) // determine some necessary variables according to sphere fields
								{									
									n[0] = Ron[0] - objects[best_i]->sphere.position[0];
									n[1] = Ron[1] - objects[best_i]->sphere.position[1]; // sets normal to the Ron vector minus the closest object's (sphere in this case) position
									n[2] = Ron[2] - objects[best_i]->sphere.position[2];
									
									normalize(n);
									
									l[0] = Rdn[0];
									l[1] = Rdn[1]; // sets l vector to the Rdn vector
									l[2] = Rdn[2];
									
									normalize(l);
									
									// calculating reflection variable
									double temp_scalar = 2.0 * ((n[0]*l[0]) + (n[1]*l[1]) + (n[2]*l[2]));
									double temp_vector[3];
									temp_vector[0] = n[0] * temp_scalar;
									temp_vector[1] = n[1] * temp_scalar;
									temp_vector[2] = n[2] * temp_scalar;	
									r[0] = temp_vector[0] - l[0];
									r[1] = temp_vector[1] - l[1];
									r[2] = temp_vector[2] - l[2];					
									// end of calculating reflection variable
									
									v[0] = Rd[0];
									v[1] = Rd[1]; // sets v vector to the Rd vector
									v[2] = Rd[2];
									
									nv[0] = v[0] * -1;
									nv[1] = v[1] * -1; // nv vector (v vector scaled by -1) to be passed into the specular_calculation function
									nv[2] = v[2] * -1;
									 
									// passes in corresponding variables for diffuse and specular calculators, using the diffuse/specular vectors as output
									diffuse_calculation(n, l, lights[j]->light.color, objects[best_i]->sphere.diffuse_color, diffuse);
									specular_calculation(n, l, lights[j]->light.color, objects[best_i]->sphere.specular_color, nv, r, 20, specular);
																									
								}
								else if(objects[best_i]->kind == 2) // determine some necessary variables according to plane fields
								{									
									n[0] = objects[best_i]->plane.normal[0];
									n[1] = objects[best_i]->plane.normal[1]; // sets normal to the closets object's (plane in this case) normal
									n[2] = objects[best_i]->plane.normal[2];
									
									normalize(n);
									
									l[0] = Rdn[0];
									l[1] = -Rdn[1]; // sets l vector to the Rdn vector but also inverts the y-coordinate, unlike what would be done for a plane's l vector
									l[2] = Rdn[2];
									
									normalize(l);
									
									// calculating reflection variable
									double temp_scalar = 2.0 * ((n[0]*l[0]) + (n[1]*l[1]) + (n[2]*l[2]));
									double temp_vector[3];
									temp_vector[0] = n[0] * temp_scalar;
									temp_vector[1] = n[1] * temp_scalar;
									temp_vector[2] = n[2] * temp_scalar;	
									r[0] = temp_vector[0] - l[0];
									r[1] = temp_vector[1] - l[1];
									r[2] = temp_vector[2] - l[2];					
									// end of calculating reflection variable
									
									v[0] = Rd[0];
									v[1] = Rd[1]; // sets v vector to the Rd vector
									v[2] = Rd[2];
									
									nv[0] = v[0] * -1;
									nv[1] = v[1] * -1; // nv vector (v vector scaled by -1) to be passed into the specular_calculation function
									nv[2] = v[2] * -1;
									

									// passes in corresponding variables for diffuse and specular calculators, using the diffuse/specular vectors as output
									diffuse_calculation(n, l, lights[j]->light.color, objects[best_i]->plane.diffuse_color, diffuse);
									specular_calculation(n, l, lights[j]->light.color, objects[best_i]->plane.specular_color, nv, r, 20, specular);																
								}
									
									// initializes object_direction (from object to light) vector
									double object_direction[3];
									object_direction[0] = Rdn[0] * -1;
									object_direction[1] = Rdn[1] * -1;
									object_direction[2] = Rdn[2] * -1;
									normalize(object_direction);
									
									double fang_val = fang(lights[j], object_direction, lights[j]->light.theta);									
									double frad_val = frad(lights[j], distance_to_light);
									color[0] += frad_val * fang_val * (diffuse[0] + specular[0]); 
									color[1] += frad_val * fang_val * (diffuse[1] + specular[1]); 
									color[2] += frad_val * fang_val * (diffuse[2] + specular[2]); 
								
							}
												
						}
						
						current_pixel.r = (unsigned char)(255 * clamp(color[0]));
						current_pixel.g = (unsigned char)(255 * clamp(color[1])); // sets current pixel's color values based on calculated colors in color vector (clamped)
						current_pixel.b = (unsigned char)(255 * clamp(color[2]));
						*temp_ptr = current_pixel; // sets current image_data struct in temp_ptr to current_pixel colored from object 
						temp_ptr++; // increments temp_ptr to point to next image_data struct in global buffer
						
						current_pixel.r = 0;
						current_pixel.g = 0; // resets current pixel RGB values to 0 after coloring current pixel
						current_pixel.b = 0;
					}
					else { // no dominant intersection found at current point so pixel is to be colored black
						current_pixel.r = (unsigned char)(255 * clamp(color[0]));
						current_pixel.g = (unsigned char)(255 * clamp(color[1])); // should all be 0 
						current_pixel.b = (unsigned char)(255 * clamp(color[2]));		
						*temp_ptr = current_pixel;  // sets current image_data struct in temp_ptr to current_pixel colored from object 
						temp_ptr++; // increments temp_ptr to point to next image_data struct in global buffer
					}
			}
		}	
		return;
}	

// write_image_data function takes in the output_file_name to know where to write out to
void write_image_data(char* output_file_name)
{
	FILE *fp;
	
	fp = fopen(output_file_name, "a"); // opens file to be appended to (file will be created if one does not exist)
	
	if(fp == NULL) 
	{
		fprintf(stderr, "Error: Output file couldn't be created/modified.\n");
		exit(1); // exits out of program due to error
	}
	
	// block of code which writes header information into the output file along with whitespaces accordingly
	fprintf(fp, header_buffer->file_format); 
	fprintf(fp, "%s", "\n");
	fprintf(fp, header_buffer->file_width);
	fprintf(fp, "%s", " ");
	fprintf(fp, header_buffer->file_height);
	fprintf(fp, "%s", "\n");
	fprintf(fp, header_buffer->file_maxcolor);
	fprintf(fp, "%s", "\n");
	
	// Writing of P6 data (as recommended by professor) starts here
	fclose(fp); // closes file after writing header information since P6 requires writing bytes
	fopen(output_file_name, "ab"); // opens file to be appended to in byte mode
	int i = 0; // initializes iterator variable
	image_data* temp_ptr = image_buffer; // temp ptr to image_data struct which will be used to navigate through stored pixels in the global buffer
		
	// while loop which iterates for every pixel in the file using width * height
	while(i != atoi(header_buffer->file_width) * atoi(header_buffer->file_height))
	{
		fwrite(&(*temp_ptr).r, sizeof(unsigned char), 1, fp); // writes the current pixels "r" value of an "unsigned char" byte to the file
		fwrite(&(*temp_ptr).g, sizeof(unsigned char), 1, fp); // writes the current pixels "g" value of an "unsigned char" byte to the file
		fwrite(&(*temp_ptr).b, sizeof(unsigned char), 1, fp); // writes the current pixels "b" value of an "unsigned char" byte to the file
				
		temp_ptr++; // increments temp_ptr to point to next image_data struct in global buffer
		i++;  // increments iterator variable
	}
		
	fclose(fp);
}

// function which takes in an origin ray, direction of the ray, position of the sphere object, and radius of the sphere object and determines if there's an intersection at the current point
double sphere_intersection(double* Ro, double* Rd, double* C, double r)
{
	double a = sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]);
	double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[1] * Rd[1] - Rd[1] * C[1] + Ro[2] * Rd[2] - Rd[2] * C[2]));
	double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[1]) - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);
		
	double det = sqr(b) - 4 * a * c;
	if(det < 0) return -1; // if determinant is negative then there's no sphere intersection so return -1
	
	det = sqrt(det);
	
	double t0 = (-b - det) / (2 * a);
	if(t0 > 0) return t0; // t0 indicates a sphere intersection so return it
	double t1 = (-b + det) / (2 * a);
	if(t1 > 0) return t1; // t1 indicates a sphere intersection so return it
	
	return -1; // didn't find a sphere intersection so return -1
}

// function which takes in an origin ray, direction of the ray, position of the plane object, and normal of the plane object and determines if there's an intersection at the current point
double plane_intersection(double* Ro, double* Rd, double* C, double* N)
{	
	normalize(N);
	double Vd = ((N[0] * Rd[0]) + (N[1] * Rd[1]) + (N[2] * Rd[2]));
	if(Vd == 0) // parallel ray so no intersection
	{
		return -1;
	}
	double Vo = -((N[0] * Ro[0]) + (N[1] * Ro[1]) + (N[2] * Ro[2])) + sqrt(sqr(C[0] - Ro[0]) + sqr(C[1] - Ro[1]) + sqr(C[2] - Ro[2]));

	
	double t = Vo/Vd;
		
	if(t > 0) // found plane intersection so return t
	{
		return t;
	}
	
	return -1; // didn't find a plane intersection so return -1
}

// next_c() wraps the getc() function and provides error checking and line number maintenance
int next_c(FILE* json) 
{
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}

// expect_c() checks that the next character is d.  If it is not it emits an error.
void expect_c(FILE* json, int d) 
{
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);    
}

// skip_ws() skips white space in the file.
void skip_ws(FILE* json) 
{
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}

// next_string() gets the next string from the file handle and emits an error if a string can not be obtained.
char* next_string(FILE* json) 
{
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }  
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);      
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);      
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

// function which reads next number from file, wrapped around error checking if nothing is read in
double next_number(FILE* json) 
{
  double value;
  if(fscanf(json, "%lf", &value) == 0) // error checking to make sure fscanf read in a number; will only evaluate if fscanf didn't read anything in and returned 0
  {
	  fprintf(stderr, "Error: Expected a number on line %d.\n", line);
      exit(1);	
  }
  return value;
}

// function which reads next vector from file
double* next_vector(FILE* json) 
{
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}

// normalizes given vector
void normalize(double* v) 
{
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

// squares given double
double sqr(double v) 
{
  return v*v;
}

// clamping helper function to make sure color isn't outside of 0-1 range (inclusive)
double clamp(double value)
{
    if (value < 0)
        return 0;
    else if (value > 1)
        return 1;
    else
        return value;
}

// does radial attenutation calculations and returns value accordingly
double frad(Object* light, double dl)
{
	// check for dl value being infinity?
	if(light->light.radial_a2 == 0) // invalid a2 value so change to 1 as default
		light->light.radial_a2 = 1.0;
		
	double return_value = (1.0 / ((light->light.radial_a2 * sqr(dl)) + (light->light.radial_a1 * dl) + light->light.radial_a0));
	
	return return_value;
}

// does angular attenutation calculations and returns value accordingly
double fang(Object* light, double direction[3], double theta)
{
	normalize(light->light.direction);
	if(light->light.kind_light == 0) // not spot light so return 1
		return 1.0;
		
	double v0_vl = (light->light.direction[0] * direction[0]) + (light->light.direction[1] * direction[1]) +  (light->light.direction[2] * direction[2]);
	
	if(v0_vl < cos(((theta / 180) * 3.14159))) // (theta / 180) * 3.14159 converts from degrees to radians for cos() function
		return 0;
		
	return pow(v0_vl, light->light.angular_a0);
}

// does diffuse color calculations and returns value accordingly
void diffuse_calculation(double n[3], double l[3], double il[3], double kd[3], double* output)
{
	double n_l = (n[0] * l[0]) + (n[1] * l[1]) + (n[2] * l[2]);
	// ambient constant would go here in the future
    if (n_l > 0) 
	{
        output[0] = (kd[0] * il[0]) * n_l;
        output[1] = (kd[1] * il[1]) * n_l;
        output[2] = (kd[2] * il[2]) * n_l;
    }
    else 
	{
        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
    }
}

// does specular color calculations and returns value accordingly
void specular_calculation(double n[3], double l[3], double il[3], double ks[3], double v[3], double r[3], double ns, double* output)
{
    double n_l = (n[0] * l[0]) + (n[1] * l[1]) + (n[2] * l[2]);
	double v_r = (v[0] * r[0]) + (v[1] * r[1]) + (v[2] * r[2]);
	
    if (n_l > 0 && v_r > 0) 
	{
        double vr_ns_power = pow(v_r, ns);
        output[0] = (ks[0] * il[0]) * vr_ns_power;
        output[1] = (ks[1] * il[1]) * vr_ns_power;
        output[2] = (ks[2] * il[2]) * vr_ns_power;
    }
    else 
	{
        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
    }
}
