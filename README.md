# TappO
A simple Tap List using the Brewfather® API and a CYD

# Version 
1.0 First Release

**Important** I am a brewer and author, I write books on recreating historic beers - **NOT a PROGRAMMER** - as will be obvious from inspection of the code. Although some of the fiddly stuff was produced by ChatGPT 3.5 after a lot of questioning and trial and error to get it to work.

# Limitations
TappO displays up to 7 batches that are set to **Completed** status in Brewfather® and have an **Additional Measurement** field called **'Tap'**.

# Main Prerequisites
Brewfather® v2.10.4 [BF] https://brewfather.app/ **Premium Version** - so yes you have to pay an annual subscription but it is worth every cent.

Uses an ESP32-2432S028R a Cheap Yellow Device [CYD] there are many variants out there - YMMV
I used one from AITEXM ROBOT Official Store - ESP32 Arduino LVGL WIFI&Bluetooth Development Board 2.8 " 240*320 Smart Display Screen 2.8inch LCD TFT Module With Touch WROOM   This one uses a resistive touch.

# Links
- Base64 Encode  https://www.base64encode.org/ 
- Postman   https://www.postman.com/
- Get Completed Batches from Brewfather https://api.brewfather.app/v2/batches/?include=measuredOg,measuredFg,measurements,&status=Completed
- I found Brian Lough's https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display a good resource
- Flasher https://github.com/marcelstoer/nodemcu-pyflasher/releases
  
# My YouTube channel, Buy My Books, GitHub and website:  
\-------------------------------------------------------------------------------------------------  
- https://m.youtube.com/channel/UCRhjjWS5IFHzldBhO2kyVkw/featured    Tritun Books Channel
- https://www.lulu.com/spotlight/prsymons  Books available Print on demand from Lulu
- https://prstemp.wixsite.com/tritun-books   Website  
\-------------------------------------------------------------------------------------------------
