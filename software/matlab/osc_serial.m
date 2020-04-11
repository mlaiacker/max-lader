function [ output_args ] = osc_serial(port,ch1_mult,ch2_mult)

if(strncmp(port,'COM',3)==1)
    instrreset;
    s = serial(port,'BaudRate',38400,'DataBits',8);
    s.InputBufferSize = 512;
    s.Timeout = 0.001;
    fopen(s);
    replay = 0;
else
    s =fopen(port);
    replay = 1;
end

parse_state = 1;
DATA_COUNT = 120;
data_line = '';
try 
    data_charge = evalin('base', 'data_charge');
catch ME
    data_charge = NaN(3,1000);
end
charge_index = 1;
try
    while parse_state ~= 0
        if parse_state==1
           [line, len] = fread(s,1,'char');
           if(len==0)
               if replay
                   break
               end
               pause(0.5)
               continue
           end
           if line == hex2dec('ff');
                parse_state=2;
           else
             fprintf('%c', line);
             data_line = [data_line line];
             if(line==10)
                 try
                 fields = sscanf(data_line,'%d %f %f %f %d %d %d %f %d %f');
                 if(length(fields)==10)
                     data_charge(:,int16(mod(fields(1),1000))) = [fields(2) fields(3) fields(10)];
                     if(int16(mod(fields(1),10))==0)
                         figure(2); 
                         subplot(2,1,1);plot(data_charge([1 3],:)');
                         grid on;
                         subplot(2,1,2);plot(data_charge(2,:)');
                         grid on;
                         pause(0.2)
                     end
                     assignin('base', 'data_charge', data_charge);
                 end
                 catch
                 end
                 data_line = '';
             end
           end
        end
        if parse_state==2
           [line, len] = fread(s,1,'char');
           if(len==0)
               if replay
                   break
               end
               continue
               parse_state = 1;
           end
           if line == hex2dec('af');
                parse_state=3;
           else
             fprintf('%c', line);
             parse_state = 1;
           end
        end
        if parse_state==3
           [line, len] = fread(s,1,'char');
           if(len==0)
               if replay
                   break
               end
               parse_state = 1;
               continue
           end
           if line == hex2dec('bf');
                parse_state=4;
           else
             fprintf('%c', line);
             parse_state = 1;
           end
        end
        if parse_state==4
           [dt_us, len] = fread(s,1,'uint16');
           %dt_us
           if(len==0)
               if replay
                   break
               end
                 parse_state = 1;
               continue
           end
           parse_state=parse_state+1;
        end
        if parse_state==5
           [index_trigger, len] = fread(s,2,'uint8');
           index_trigger
           if(len==0)
               if replay
                   break
               end
                 parse_state = 1;
               continue
           end
           parse_state=parse_state+1;
        end
        if parse_state==6
           [trigger_level, len] = fread(s,1,'int16');
           trigger_level
           if(len==0)
               if replay
                   break
               end
                 parse_state = 1;
               continue
           end
           parse_state=parse_state+1;
        end
        if parse_state==7
           [trigger_stuff, len] = fread(s,2,'uint8');
           if(len==0)
               if replay
                   break
               end
                 parse_state = 1;
               continue
           end
           parse_state=parse_state+1;
        end
        if parse_state==8
           [data, len] = fread(s,DATA_COUNT*2,'int16');
           new_index=[index_trigger(2)+2:DATA_COUNT,1:index_trigger(2)+1];
           
           new_index=mod([index_trigger(2)+2:DATA_COUNT,1:index_trigger(2)+1]+(DATA_COUNT/2)-1,DATA_COUNT)+1;
           output_args = reshape(data,DATA_COUNT,2);
           output_args = output_args(new_index,:);
           time = (1:DATA_COUNT)*dt_us*1e-6;
           figure(1);
           plot(time,[output_args(:,1)*ch1_mult output_args(:,2)*ch2_mult ones(DATA_COUNT,1)*trigger_level*ch2_mult],'+-')
           grid on
           legend('ch1','ch2','trigger')
           pause(0.5)
           if(len==0)
               if replay
                   break
               end
                 parse_state = 1;
               continue
           end
           parse_state=1;
        end
    end
catch ME
    fclose(s);
    rethrow(ME)
end

fclose(s);
end