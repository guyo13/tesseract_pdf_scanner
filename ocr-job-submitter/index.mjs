import { BatchClient, SubmitJobCommand } from "@aws-sdk/client-batch";
import { S3Client, GetObjectAttributesCommand, GetObjectCommand } from "@aws-sdk/client-s3";
import { getSignedUrl } from "@aws-sdk/s3-request-presigner";
import util from 'util';


const batchClient = new BatchClient();
const s3Client = new S3Client();
const ObjectAttributes = [ "ETag",  "Checksum", "ObjectParts", "StorageClass" , "ObjectSize"];
// TODO - add ARN
const JOB_DEF_ARN = "";
const JOB_QUEUE_ARN = "";

function extractS3Key(s3Record) {
    return decodeURIComponent(s3Record.s3.object.key.replace(/\+/g, ' '))
}

async function verifyObjectExists(s3Record) {
    const bucket = s3Record.s3.bucket.name;
    const key = extractS3Key(s3Record);
    const params = {
        Bucket: bucket,
        Key: key,
        ObjectAttributes,
    };
    const getCommand = new GetObjectAttributesCommand(params);
    const response = await s3Client.send(getCommand);
    // TODO - catch NoSuchKey
    console.log(util.inspect(response, true, 100));

    return Boolean(response);
}

function getCurrentDateFormatted() {
    let now = new Date();
    let year = now.getFullYear();
    let month = String(now.getMonth() + 1).padStart(2, '0');
    let day = String(now.getDate()).padStart(2, '0');
    let hours = String(now.getHours()).padStart(2, '0');
    let minutes = String(now.getMinutes()).padStart(2, '0');
    let seconds = String(now.getSeconds()).padStart(2, '0');

    return `${year}-${month}-${day}-${hours}-${minutes}-${seconds}`;
}

function generateJobName(key) {
    return `OCR-${key.replaceAll(/\..+$/g, "")}-${getCurrentDateFormatted()}`;
}

function generateS3PresignedUrl(getObjectParams) {
    const command = new GetObjectCommand(getObjectParams);
    return getSignedUrl(s3Client, command, { expiresIn: 3600 });
}

async function submitBatchJob(s3Record) {
    const bucket = s3Record.s3.bucket.name;
    const key = extractS3Key(s3Record);
    const getObjectParams = {
        Bucket: bucket,
        Key: key
    }
    const url = await generateS3PresignedUrl(getObjectParams);
    const jobName = generateJobName(key);
    const input = {
              jobName,
              jobDefinition: JOB_DEF_ARN,
              jobQueue: JOB_QUEUE_ARN,
              parameters: {
                url,
                key
              },
            }

    console.log("Submmiting job", util.inspect(input, true, 100));
    const command = new SubmitJobCommand(input);
    
    return batchClient.send(command);
}

export const handler = async (event, context) => {
    console.log(util.inspect(event, true, 100));
    const s3Record = event.Records[0];
    try {
        await verifyObjectExists(s3Record);
        const resp = await submitBatchJob(s3Record);
        console.log('resp', util.inspect(resp, false, 100))
        return {status: 'ok'};
    } catch (err) {
        console.log(err);
        throw new Error(err);
    }
};

