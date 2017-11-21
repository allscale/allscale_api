require 'rmagick'
include Magick

#Get list of images at the current directory
all_images = Dir["animation_*.png"]

# Sort the image by the number in "animation_[NUMBER].png"
all_images = all_images.sort { | a,b | Integer(a.split('_')[1].split('.')[0]) <=> Integer(b.split('_')[1].split('.')[0]) }

n = all_images.size / 200
if n < 1 then
    n = 1
end

#Ignore some of the images to get ~200 images
n = 1
images = (n - 1).step(all_images.size - 1, n).map { | i | all_images[i] }

#Write gif file if there is at least one image in the directory
if images.size > 0 then
    animation = ImageList.new(*images)
    animation.delay = 1
    animation.write("animated.gif")
else
    puts "No \"animation_*.png\" files found"
end