// Copyright 2018 Nicholas Honton, all rights reserved
// Program Purpose: Create .avs and .clt files
// to make for simpler and more straightforward video encoding

// includes
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

// Makefiles class: contains members and functions to take information from input.txt
// and convert it into requested files
struct makefiles
{
	// members
	std::string filename_d2v; // what the output file names will be
	std::string filename_subs; // what the main subtitle file name will be
	std::string filename_karaoke; // what the karaoke filename will be
	std::string splash_location; // location of the video's splash image
	std::string video_quality; // the quality, of the video, as it were
	std::vector<int> data_ep; // contains trim times for the episode
	std::vector<int> data_cm; // contains trim times for the commercials
	std::vector<std::string> tln; // contains any and all strings for sources for TL notes

	// functions
	makefiles(); // constructor
	void print_to_file(); // printing to files
	void final_message(); // prints something pseudo-randomly when operation has completed
};

// struct constructor (heh)
// reads contents of input.txt
// uses getline to read into appropriate strings
// then reads the rest into the vectors
makefiles::makefiles()
{
	// open input.txt
	std::ifstream input;
	input.open("input.txt");
	// get the filename_d2v and the Splash Location
	std::getline(input, filename_d2v);
	std::getline(input, filename_subs);
	std::getline(input, filename_karaoke);
	std::getline (input, splash_location);
	std::getline(input, video_quality);
	// loop until end of file retrieving remaining information
	while (!input.eof())
	{
		// get next line from input
		std::string s;
		std::getline(input, s);
		// ignore any empty lines
		if (s.length() <= 1)
		{
			continue;
		}
		// if it's a long string, it's a TL note
		if (s.length() > 6)
		{
			tln.push_back(s);
		}
		else if (s == "CM")
		{
			break; // end this loop to get to next loop for CM times
		}
		else // if it's a short string it's a trim time
		{
			data_ep.push_back(std::stoi(s));
		}
	}
	// another loop, for CM times
	while (!input.eof())
	{
		std::string s;
		std::getline(input, s);
		// ignore any empty lines
		if (s.length() <= 1)
		{
			continue;
		}
		// if it's a long string, it's a TL note
		if (s.length() > 6)
		{
			tln.push_back(s);
		}
		else // if it's a short string it's a trim time
		{
			data_cm.push_back(std::stoi(s));
		}
	}
	input.close();
	// make sure video quality string is capitalized for easy checking later
	for (unsigned int i = 0; i < video_quality.length(); ++i)
	{
		video_quality[i] = toupper(video_quality[i]);
	}
}

// print_to_file:
// taking the information from input.txt and putting it into files accordingly
void makefiles::print_to_file()
{
	// PHASE 1
	// create output streams
	// create strings have information added until printed at the end of phase
	std::string avs = "";
	std::string clt = "";
	std::ofstream output_avs;
	std::ofstream output_clt;
	// start inserting information into avs
	avs += "DGDecode_mpeg2source(\"";
	avs += filename_d2v;
	avs += ".d2v\", info=3)\n#b = b.ColorMatrix(hints=true, interlaced=true, threads=0)\n#b = b.tfm(order=1).tdecimate(rate=23.976)\ntfm()\nsaved=last\ncrop(0,150,-0,-400)\nTDecimate(clip2=saved)\nSpline36Resize(1920,1080)\nChangeFPS(23.976)\nTextSubMod(\"";
	avs += filename_subs;
	avs += "\")\nTextSubMod(\"";
	avs += filename_karaoke;
	avs += "\")\n\n";
	// into clt
	clt += "<?xml version=\"1.0\"?>\n<Cuts xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <Framerate>23.976</Framerate>\n  <Style>NO_TRANSITION</Style>\n  <AllCuts>";
	// insert input from file in a loop
	int t = 0;
	if (video_quality != "QC")
	{
		avs += "__film = last";
		// if there's no times to add, loop does not start
		while (!data_ep.empty())
		{
			// edit filename.avs

			avs = avs + "\n__t" + std::to_string(t) + " = __film.trim(";
			avs = avs + std::to_string(data_ep[0]) + ", " + std::to_string(data_ep[1]) + ")";
			++t;

			// edit filename_a.clt
			clt += "\n    <CutSection>\n      <startFrame>";
			clt += std::to_string(data_ep[0]);
			clt += "</startFrame>\n      <endFrame>";
			clt += std::to_string(data_ep[1]);
			clt += "</endFrame>\n    </CutSection>";
			// clear out two used values from data_ep
			// loop ends once all values have been used
			data_ep.erase(data_ep.begin());
			data_ep.erase(data_ep.begin());
		}
		// one more loop for avs for t-adding string
		avs += "\n\nc = ";
		for (int i = 0; i < t; ++i)
		{
			avs += "__t" + std::to_string(i);
			if (i + 1 == t)
			{
				avs += "\nc\n\n";
			}
			else
			{
				avs += " ++ ";
			}
		}
	}
	// conclude adding data to filename.avs and close it
	// if not a QC copy, add the splash
	if (video_quality != "QC" && video_quality != "FQC" && splash_location != "")
	{
		avs += "a = ImageSource(file=\"" + splash_location + "\",end=71,fps=23.976).ConvertToYV12";
		avs += "\na ++ c";
	}
	// add TL notes, if applicable
	if (!tln.empty())
	{
		avs += "\n";
		while (!tln.empty()) // add on new lines until everything from tln has been added
		{
			avs += "\n__film = last\nc = __film ++ ImageSource(file = \"";
			avs += tln[0];
			avs += "\",end=71,fps=23.976).ConvertToYV12\nc";
			tln.erase(tln.begin());
		}
	}
	avs += "\n\n";
	// resize video accordingly
	if (video_quality == "HD720" || video_quality == "FQC" || video_quality == "QC")
	{
		avs += "Spline36Resize(1280,720)";
		avs += "\nassumefps(23.976)";
		output_avs.open(filename_d2v + video_quality + ".avs");
		output_avs << avs;
		output_avs.close();
	}
	else if (video_quality == "SD")
	{
		avs += "Spline36Resize(848, 480)";
		avs += "\nassumefps(23.976)";
		output_avs.open(filename_d2v + "SD.avs");
		output_avs << avs;
		output_avs.close();
	}
	else if (video_quality == "HD")
	{
		avs += "\nassumefps(23.976)";
		output_avs.open(filename_d2v + "HD.avs");
		output_avs << avs;
		output_avs.close();

	}
	else if (video_quality == "DISTRO")
	{
		// create three seperate files
		std::string HD_AVS = avs;
		std::string HD_720_AVS = avs;
		std::string SD_AVS = avs;
		std::ofstream output_avs_HD;
		std::ofstream output_avs_SD;
		// 720p
		HD_720_AVS += "Spline36Resize(1280,720)";
		HD_720_AVS += "\nassumefps(23.976)";
		output_avs.open(filename_d2v + "HD720.avs");
		output_avs << HD_720_AVS;
		output_avs.close();
		// 1080p
		HD_AVS += "\nassumefps(23.976)";
		output_avs_HD.open(filename_d2v + "HD.avs");
		output_avs_HD << HD_AVS;
		output_avs_HD.close();
		// 480p
		SD_AVS += "Spline36Resize(848, 480)";
		SD_AVS += "\nassumefps(23.976)";
		output_avs_SD.open(filename_d2v + "SD.avs");
		output_avs_SD << SD_AVS;
		output_avs_SD.close();
	}

	// conclude adding data to filenamea.clt and close it
	output_clt << "\n  </AllCuts>\n</Cuts>";
	output_clt.open(filename_d2v + "a.clt");
	output_clt << clt;
	output_clt.close();

	// PART 2 : CM FILES

	if (video_quality != "QC")
	{
		// create new strings
		std::string avs_cm = "";
		std::string clt_cm = "";

		// add static information into avs_cm
		avs_cm += "DGDecode_mpeg2source(\"";
		avs_cm += filename_d2v;
		avs_cm += ".d2v\", info=3)\n#b = b.ColorMatrix(hints=true, interlaced=true, threads=0)\n#b = b.tfm(order=1).tdecimate(rate=23.976)\ntfm()\nsaved=last\ncrop(0,150,-0,-400)\nTDecimate(clip2=saved)\nSpline36Resize(1920,1080)\nChangeFPS(23.976)\nTextSubMod(\"";
		avs_cm += filename_subs;
		avs_cm += "\")\nTextSubMod(\"";
		avs_cm += filename_karaoke;
		avs_cm += "\")\n\n__film = last";

		// add static information into clt
		clt_cm += "<?xml version=\"1.0\"?>\n<Cuts xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n  <Framerate>23.976</Framerate>\n  <Style>NO_TRANSITION</Style>\n  <AllCuts>";

		// insert input from file in a loop
		t = 0; // reset T-value
		while (!data_cm.empty()) // loop ends when read-in numbers are exhausted
		{
			// add trim times to avs_cm
			avs_cm += "\n__t" + std::to_string(t) + " = __film.trim(";
			avs_cm += std::to_string(data_cm[0]) + ", " + std::to_string(data_cm[1]) + ")";
			++t;

			// add cut frame times to clt_cm
			clt_cm += "\n    <CutSection>\n      <startFrame>";
			clt_cm += std::to_string(data_cm[0]);
			clt_cm += "</startFrame>\n      <endFrame>";
			clt_cm += std::to_string(data_cm[1]);
			clt_cm += "</endFrame>\n    </CutSection>";
			data_cm.erase(data_cm.begin());
			data_cm.erase(data_cm.begin());
		}

		// one more loop for .avs for t-values
		avs_cm += "\n\nc = ";
		for (int i = 0; i < t; ++i)
		{
			avs_cm += "__t" + std::to_string(i);
			if (i + 1 == t)
			{
				avs_cm += "\nc\n\n";
			}
			else
			{
				avs_cm += " ++ ";
			}
		}

		// conclude adding data to filename CM.avs and close it
		// resize accordingly
		if (video_quality == "HD720" || video_quality == "FQC" || video_quality == "QC")
		{
			avs_cm += "Spline36Resize(1280,720)";
			avs_cm += "\nassumefps(23.976)";
			output_avs.open(filename_d2v + "CM" + video_quality + ".avs");
			output_avs << avs_cm;
			output_avs.close();
		}
		else if (video_quality == "SD")
		{
			avs_cm += "Spline36Resize(848, 480)";
			avs_cm += "\nassumefps(23.976)";
			output_avs.open(filename_d2v + "CM" + video_quality + ".avs");
			output_avs << avs_cm;
			output_avs.close();
		}
		else if (video_quality == "HD")
		{
			avs_cm += "\nassumefps(23.976)";
			output_avs.open(filename_d2v + "CM" + video_quality + ".avs");
			output_avs << avs_cm;
			output_avs.close();
		}
		else if (video_quality == "DISTRO")
		{
			// create streams and different strings
			std::string AVS_HD_720 = avs_cm;
			std::string AVS_HD = avs_cm;
			std::string AVS_SD = avs_cm;
			std::ofstream output_avs_HD;
			std::ofstream output_avs_SD;
			// 720p not required for CMs
			// 1080p
			AVS_HD += "\nassumefps(23.976)";
			output_avs_HD.open(filename_d2v + "CMHD.avs");
			output_avs_HD << AVS_HD;
			output_avs_HD.close();
			// 480p
			AVS_SD += "Spline36Resize(848, 480)";
			AVS_SD += "\nassumefps(23.976)";
			output_avs_SD.open(filename_d2v + "CMSD.avs");
			output_avs_SD << AVS_SD;
			output_avs_SD.close();
		}
		avs_cm += "\nassumefps(23.976)";
		output_avs.close();
		// conclude adding data to filenamea.clt and close it
		
		clt_cm += "\n  </AllCuts>\n</Cuts>";
		if (video_quality != "QC") // no cuts for QC copies
		{
			output_clt.open(filename_d2v + " CMa.clt");
			output_clt << clt_cm;
			output_clt.close();
		}
	}
}

// randomizes message that prints once operations are complete
// based upon user input
void makefiles::final_message()
{
	std::vector<std::string> v;
	v.push_back("STANDING BY.....COMPLETE.");
	v.push_back("READY.....EXCEED CHARGE.");
	v.push_back("JET SLIGER, COME CLOSER.");
	v.push_back("SIDE BASSHAR, TAKE OFF.");
	v.push_back("AUTO VAIJIN, GET INTO THE ACTION.");
	v.push_back("START UP.....TIME OUT.");
	int c = 0;
	for (unsigned int i = 0; i < filename_d2v.length(); ++i)
	{
		c += filename_d2v[i];
	}
	c = c % 6;
	std::cout << v[c] << std::endl;
}


// main creates class, calls functions, ends
int main()
{
	makefiles m;
	m.print_to_file();
	m.final_message();
    return 0;
}

