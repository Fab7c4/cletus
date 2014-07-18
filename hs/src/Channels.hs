{-# OPTIONS_GHC -Wall #-}

module Channels ( chanRc
                , chanSimTelem
                , chanSensors
                ) where

chanRc :: String
chanRc = "ipc:///tmp/rc"

chanSimTelem :: String
chanSimTelem = "ipc:///tmp/simtelem"

chanSensors :: String
chanSensors = "ipc:///tmp/sensors"
